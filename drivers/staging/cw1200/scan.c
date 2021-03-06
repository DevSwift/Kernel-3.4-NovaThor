/*
 * Scan implementation for ST-Ericsson CW1200 mac80211 drivers
 *
 * Copyright (c) 2010, ST-Ericsson
 * Author: Dmitry Tarnyagin <dmitry.tarnyagin@stericsson.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/sched.h>
#include "cw1200.h"
#include "scan.h"
#include "sta.h"
#include "pm.h"

static void cw1200_scan_restart_delayed(struct cw1200_common *priv);

static int cw1200_scan_start(struct cw1200_common *priv, struct wsm_scan *scan)
{
	int ret, i;
	int tmo = 2000;

	for (i = 0; i < scan->numOfChannels; ++i)
		tmo += scan->ch[i].maxChannelTime + 10;

	atomic_set(&priv->scan.in_progress, 1);
	cw1200_pm_stay_awake(&priv->pm_state, tmo * HZ / 1000);
	queue_delayed_work(priv->workqueue, &priv->scan.timeout,
		tmo * HZ / 1000);
	ret = wsm_scan(priv, scan);
	if (unlikely(ret)) {
		atomic_set(&priv->scan.in_progress, 0);
		cancel_delayed_work_sync(&priv->scan.timeout);
		cw1200_scan_restart_delayed(priv);
	}
	return ret;
}

int cw1200_hw_scan(struct ieee80211_hw *hw,
		   struct ieee80211_vif *vif,
		   struct cfg80211_scan_request *req)
{
	struct cw1200_common *priv = hw->priv;
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PROBE_REQUEST,
	};
	int i;

	if (!priv->vif)
		return -EINVAL;

	/* Scan when P2P_GO corrupt firmware MiniAP mode */
	if (priv->join_status == CW1200_JOIN_STATUS_AP)
		return -EOPNOTSUPP;

	if (req->n_ssids == 1 && !req->ssids[0].ssid_len)
		req->n_ssids = 0;

	wiphy_dbg(hw->wiphy, "[SCAN] Scan request for %d SSIDs.\n",
		req->n_ssids);

	if (req->n_ssids > WSM_SCAN_MAX_NUM_OF_SSIDS)
		return -EINVAL;

	frame.skb = ieee80211_probereq_get(hw, priv->vif, NULL, 0,
		req->ie, req->ie_len);
	if (!frame.skb)
		return -ENOMEM;

	/* will be unlocked in cw1200_scan_work() */
	down(&priv->scan.lock);
	mutex_lock(&priv->conf_mutex);
	if (frame.skb) {
		int ret = wsm_set_template_frame(priv, &frame);
		if (0 == ret) {
			/*
			* set empty probe response template in order
			* to receive probe requests from firmware
			*/
			frame.frame_type = WSM_FRAME_TYPE_PROBE_RESPONSE;
			frame.disable = true;
			ret = wsm_set_template_frame(priv, &frame);
		}
		if (ret) {
			mutex_unlock(&priv->conf_mutex);
			up(&priv->scan.lock);
			dev_kfree_skb(frame.skb);
			return ret;
		}
	}

	wsm_lock_tx(priv);

	BUG_ON(priv->scan.req);
	priv->scan.req = req;
	priv->scan.n_ssids = 0;
	priv->scan.status = 0;
	priv->scan.begin = &req->channels[0];
	priv->scan.curr = priv->scan.begin;
	priv->scan.end = &req->channels[req->n_channels];
	priv->scan.output_power = priv->output_power;

	for (i = 0; i < req->n_ssids; ++i) {
		struct wsm_ssid *dst =
			&priv->scan.ssids[priv->scan.n_ssids];
		BUG_ON(req->ssids[i].ssid_len > sizeof(dst->ssid));
		memcpy(&dst->ssid[0], req->ssids[i].ssid,
			sizeof(dst->ssid));
		dst->length = req->ssids[i].ssid_len;
		++priv->scan.n_ssids;
	}

	mutex_unlock(&priv->conf_mutex);

	if (frame.skb)
		dev_kfree_skb(frame.skb);
	queue_work(priv->workqueue, &priv->scan.work);
	return 0;
}

void cw1200_scan_work(struct work_struct *work)
{
	struct cw1200_common *priv = container_of(work, struct cw1200_common,
							scan.work);
	struct ieee80211_channel **it;
	struct wsm_scan scan = {
		.scanType = WSM_SCAN_TYPE_FOREGROUND,
		.scanFlags = WSM_SCAN_FLAG_SPLIT_METHOD,
	};
	bool first_run = priv->scan.begin == priv->scan.curr &&
			priv->scan.begin != priv->scan.end;
	int i;

	if (first_run) {
		/* Firmware gets crazy if scan request is sent
		 * when STA is joined but not yet associated.
		 * Force unjoin in this case. */
		if (cancel_delayed_work_sync(&priv->join_timeout) > 0)
			cw1200_join_timeout(&priv->join_timeout.work);
	}

	mutex_lock(&priv->conf_mutex);

	if (first_run) {
		if (priv->join_status == CW1200_JOIN_STATUS_STA &&
				!(priv->powersave_mode.pmMode & WSM_PSM_PS)) {
			struct wsm_set_pm pm = priv->powersave_mode;
			pm.pmMode = WSM_PSM_PS;
			cw1200_set_pm(priv, &pm);
		} else if (priv->join_status == CW1200_JOIN_STATUS_MONITOR) {
			/* FW bug: driver has to restart p2p-dev mode
			 * after scan */
			cw1200_disable_listening(priv);
		}
	}

	if (!priv->scan.req || (priv->scan.curr == priv->scan.end)) {
		if (priv->scan.output_power != priv->output_power)
			WARN_ON(wsm_set_output_power(priv,
						priv->output_power * 10));
		if (priv->join_status == CW1200_JOIN_STATUS_STA &&
				!(priv->powersave_mode.pmMode & WSM_PSM_PS))
			cw1200_set_pm(priv, &priv->powersave_mode);

		if (priv->scan.status < 0)
			wiphy_dbg(priv->hw->wiphy,
					"[SCAN] Scan failed (%d).\n",
					priv->scan.status);
		else if (priv->scan.req)
			wiphy_dbg(priv->hw->wiphy,
					"[SCAN] Scan completed.\n");
		else
			wiphy_dbg(priv->hw->wiphy,
					"[SCAN] Scan canceled.\n");

		priv->scan.req = NULL;
		cw1200_scan_restart_delayed(priv);
		wsm_unlock_tx(priv);
		mutex_unlock(&priv->conf_mutex);
		ieee80211_scan_completed(priv->hw, priv->scan.status ? 1 : 0);
		up(&priv->scan.lock);
		return;
	} else {
		struct ieee80211_channel *first = *priv->scan.curr;
		for (it = priv->scan.curr + 1, i = 1;
		     it != priv->scan.end && i < WSM_SCAN_MAX_NUM_OF_CHANNELS;
		     ++it, ++i) {
			if ((*it)->band != first->band)
				break;
			if (((*it)->flags ^ first->flags) &
					IEEE80211_CHAN_PASSIVE_SCAN)
				break;
			if (!(first->flags & IEEE80211_CHAN_PASSIVE_SCAN) &&
			    (*it)->max_power != first->max_power)
				break;
		}
		scan.band = first->band;

		if (priv->scan.req->no_cck)
			scan.maxTransmitRate = WSM_TRANSMIT_RATE_6;
		else
			scan.maxTransmitRate = WSM_TRANSMIT_RATE_1;
		/* TODO: Is it optimal? */
		scan.numOfProbeRequests =
			(first->flags & IEEE80211_CHAN_PASSIVE_SCAN) ? 0 : 2;
		scan.numOfSSIDs = priv->scan.n_ssids;
		scan.ssids = &priv->scan.ssids[0];
		scan.numOfChannels = it - priv->scan.curr;
		/* TODO: Is it optimal? */
		scan.probeDelay = 100;
		/* It is not stated in WSM specification, however
		 * FW team says that driver may not use FG scan
		 * when joined. */
		if (priv->join_status == CW1200_JOIN_STATUS_STA) {
			scan.scanType = WSM_SCAN_TYPE_BACKGROUND;
			scan.scanFlags = WSM_SCAN_FLAG_FORCE_BACKGROUND;
		}
		scan.ch = kzalloc(
			sizeof(struct wsm_scan_ch[it - priv->scan.curr]),
			GFP_KERNEL);
		if (!scan.ch) {
			priv->scan.status = -ENOMEM;
			goto fail;
		}
		for (i = 0; i < scan.numOfChannels; ++i) {
			scan.ch[i].number = priv->scan.curr[i]->hw_value;
			scan.ch[i].minChannelTime = 50;
			scan.ch[i].maxChannelTime = 110;
		}
		if (!(first->flags & IEEE80211_CHAN_PASSIVE_SCAN) &&
		    priv->scan.output_power != first->max_power) {
			priv->scan.output_power = first->max_power;
			WARN_ON(wsm_set_output_power(priv,
						priv->scan.output_power * 10));
		}
		priv->scan.status = cw1200_scan_start(priv, &scan);
		kfree(scan.ch);
		if (priv->scan.status)
			goto fail;
		priv->scan.curr = it;
	}
	mutex_unlock(&priv->conf_mutex);
	return;

fail:
	priv->scan.curr = priv->scan.end;
	mutex_unlock(&priv->conf_mutex);
	queue_work(priv->workqueue, &priv->scan.work);
	return;
}

static void cw1200_scan_restart_delayed(struct cw1200_common *priv)
{
	if (priv->delayed_link_loss) {
		int tmo = priv->cqm_beacon_loss_count;

		if (priv->scan.direct_probe)
			tmo = 0;

		priv->delayed_link_loss = 0;
		/* Restart beacon loss timer and requeue
		   BSS loss work. */
		wiphy_dbg(priv->hw->wiphy,
				"[CQM] Requeue BSS loss in %d "
				"beacons.\n", tmo);
		spin_lock(&priv->bss_loss_lock);
		priv->bss_loss_status = CW1200_BSS_LOSS_NONE;
		spin_unlock(&priv->bss_loss_lock);
		cancel_delayed_work_sync(&priv->bss_loss_work);
		queue_delayed_work(priv->workqueue,
				&priv->bss_loss_work,
				tmo * HZ / 10);
	}

	/* FW bug: driver has to restart p2p-dev mode after scan. */
	if (priv->join_status == CW1200_JOIN_STATUS_MONITOR) {
		cw1200_enable_listening(priv);
		cw1200_update_filtering(priv);
	}

	if (priv->delayed_unjoin) {
		priv->delayed_unjoin = false;
		if (queue_work(priv->workqueue, &priv->unjoin_work) <= 0)
			wsm_unlock_tx(priv);
	}
}

static void cw1200_scan_complete(struct cw1200_common *priv)
{
	if (priv->scan.direct_probe) {
		wiphy_dbg(priv->hw->wiphy, "[SCAN] Direct probe complete.\n");
		cw1200_scan_restart_delayed(priv);
		priv->scan.direct_probe = 0;
		up(&priv->scan.lock);
		wsm_unlock_tx(priv);
	} else {
		cw1200_scan_work(&priv->scan.work);
	}
}

void cw1200_scan_complete_cb(struct cw1200_common *priv,
				struct wsm_scan_complete *arg)
{
	if (unlikely(priv->mode == NL80211_IFTYPE_UNSPECIFIED))
		/* STA is stopped. */
		return;

	if (cancel_delayed_work_sync(&priv->scan.timeout) > 0) {
		priv->scan.status = 1;
		queue_delayed_work(priv->workqueue,
				&priv->scan.timeout, 0);
	}
}

void cw1200_scan_timeout(struct work_struct *work)
{
	struct cw1200_common *priv =
		container_of(work, struct cw1200_common, scan.timeout.work);
	if (likely(atomic_xchg(&priv->scan.in_progress, 0))) {
		if (priv->scan.status > 0)
			priv->scan.status = 0;
		else if (!priv->scan.status) {
			wiphy_warn(priv->hw->wiphy,
				"Timeout waiting for scan "
				"complete notification.\n");
			priv->scan.status = -ETIMEDOUT;
			priv->scan.curr = priv->scan.end;
			WARN_ON(wsm_stop_scan(priv));
		}
		cw1200_scan_complete(priv);
	}
}

void cw1200_probe_work(struct work_struct *work)
{
	struct cw1200_common *priv =
		container_of(work, struct cw1200_common, scan.probe_work.work);
	u8 queueId = cw1200_queue_get_queue_id(priv->pending_frame_id);
	struct cw1200_queue *queue = &priv->tx_queue[queueId];
	const struct cw1200_txpriv *txpriv;
	struct wsm_tx *wsm;
	struct wsm_template_frame frame = {
		.frame_type = WSM_FRAME_TYPE_PROBE_REQUEST,
	};
	struct wsm_ssid ssids[1] = {{
		.length = 0,
	} };
	struct wsm_scan_ch ch[1] = {{
		.minChannelTime = 0,
		.maxChannelTime = 10,
	} };
	struct wsm_scan scan = {
		.scanType = WSM_SCAN_TYPE_FOREGROUND,
		.numOfProbeRequests = 1,
		.probeDelay = 0,
		.numOfChannels = 1,
		.ssids = ssids,
		.ch = ch,
	};
	u8 *ies;
	size_t ies_len;
	int ret;

	wiphy_dbg(priv->hw->wiphy, "[SCAN] Direct probe work.\n");

	BUG_ON(queueId >= 4);
	BUG_ON(!priv->channel);

	mutex_lock(&priv->conf_mutex);
	if (unlikely(down_trylock(&priv->scan.lock))) {
		/* Scan is already in progress. Requeue self. */
		schedule();
		queue_delayed_work(priv->workqueue,
					&priv->scan.probe_work, HZ / 10);
		mutex_unlock(&priv->conf_mutex);
		return;
	}

	if (cw1200_queue_get_skb(queue,	priv->pending_frame_id,
			&frame.skb, &txpriv)) {
		up(&priv->scan.lock);
		mutex_unlock(&priv->conf_mutex);
		wsm_unlock_tx(priv);
		return;
	}
	wsm = (struct wsm_tx *)frame.skb->data;
	scan.maxTransmitRate = wsm->maxTxRate;
	scan.band = (priv->channel->band == IEEE80211_BAND_5GHZ) ?
		WSM_PHY_BAND_5G : WSM_PHY_BAND_2_4G;
	if (priv->join_status == CW1200_JOIN_STATUS_STA) {
		scan.scanType = WSM_SCAN_TYPE_BACKGROUND;
		scan.scanFlags = WSM_SCAN_FLAG_FORCE_BACKGROUND;
	}
	ch[0].number = priv->channel->hw_value;

	skb_pull(frame.skb, txpriv->offset);

	ies = &frame.skb->data[sizeof(struct ieee80211_hdr_3addr)];
	ies_len = frame.skb->len - sizeof(struct ieee80211_hdr_3addr);

	if (ies_len) {
		u8 *ssidie =
			(u8 *)cfg80211_find_ie(WLAN_EID_SSID, ies, ies_len);
		if (ssidie && ssidie[1] && ssidie[1] <= sizeof(ssids[0].ssid)) {
			u8 *nextie = &ssidie[2 + ssidie[1]];
			/* Remove SSID from the IE list. It has to be provided
			 * as a separate argument in cw1200_scan_start call */

			/* Store SSID localy */
			ssids[0].length = ssidie[1];
			memcpy(ssids[0].ssid, &ssidie[2], ssids[0].length);
			scan.numOfSSIDs = 1;

			/* Remove SSID from IE list */
			ssidie[1] = 0;
			memmove(&ssidie[2], nextie, &ies[ies_len] - nextie);
			skb_trim(frame.skb, frame.skb->len - ssids[0].length);
		}
	}

	/* FW bug: driver has to restart p2p-dev mode after scan */
	if (priv->join_status == CW1200_JOIN_STATUS_MONITOR)
		cw1200_disable_listening(priv);
	ret = WARN_ON(wsm_set_template_frame(priv, &frame));
	priv->scan.direct_probe = 1;
	if (!ret) {
		wsm_flush_tx(priv);
		ret = WARN_ON(cw1200_scan_start(priv, &scan));
	}
	mutex_unlock(&priv->conf_mutex);

	skb_push(frame.skb, txpriv->offset);
	if (!ret)
		IEEE80211_SKB_CB(frame.skb)->flags |= IEEE80211_TX_STAT_ACK;
	BUG_ON(cw1200_queue_remove(queue, priv->pending_frame_id));

	if (ret) {
		priv->scan.direct_probe = 0;
		up(&priv->scan.lock);
		wsm_unlock_tx(priv);
	}

	return;
}
