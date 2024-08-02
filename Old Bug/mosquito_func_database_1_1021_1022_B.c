static int db__message_write_inflight_out_single(struct mosquitto_db *db, struct mosquitto *context, struct mosquitto_client_msg *msg)
{
	mosquitto_property *cmsg_props = NULL, *store_props = NULL;
	int rc;
	uint16_t mid;
	int retries;
	int retain;
	const char *topic;
	uint8_t qos;
	uint32_t payloadlen;
	const void *payload;
	time_t now = 0;
	uint32_t expiry_interval;

	expiry_interval = 0;
	if(msg->store->message_expiry_time){
		if(now == 0){
			now = time(NULL);
		}
		if(now > msg->store->message_expiry_time){
			/* Message is expired, must not send. */
			db__message_remove(db, &context->msgs_out, msg);
			if(msg->direction == mosq_md_out && msg->qos > 0){
				util__increment_send_quota(context);
			}
			return MOSQ_ERR_SUCCESS;
		}else{
			expiry_interval = (uint32_t)(msg->store->message_expiry_time - now);
		}
	}
	mid = msg->mid;
	retries = msg->dup;
	retain = msg->retain;
	topic = msg->store->topic;
	qos = (uint8_t)msg->qos;
	payloadlen = msg->store->payloadlen;
	payload = UHPA_ACCESS_PAYLOAD(msg->store);
	cmsg_props = msg->properties;
	store_props = msg->store->properties;

	switch(msg->state){
		case mosq_ms_publish_qos0:
			rc = send__publish(context, mid, topic, payloadlen, payload, qos, retain, retries, cmsg_props, store_props, expiry_interval);
			if(rc == MOSQ_ERR_SUCCESS || rc == MOSQ_ERR_OVERSIZE_PACKET){
				db__message_remove(db, &context->msgs_out, msg);
			}else{
				return rc;
			}
			break;

		case mosq_ms_publish_qos1:
			rc = send__publish(context, mid, topic, payloadlen, payload, qos, retain, retries, cmsg_props, store_props, expiry_interval);
			if(rc == MOSQ_ERR_SUCCESS){
				msg->timestamp = mosquitto_time();
				msg->dup = 1; /* Any retry attempts are a duplicate. */
				msg->state = mosq_ms_wait_for_puback;
			}else if(rc == MOSQ_ERR_OVERSIZE_PACKET){
				db__message_remove(db, &context->msgs_out, msg);
			}else{
				return rc;
			}
			break;

		case mosq_ms_publish_qos2:
			rc = send__publish(context, mid, topic, payloadlen, payload, qos, retain, retries, cmsg_props, store_props, expiry_interval);
			if(rc == MOSQ_ERR_SUCCESS){
				msg->timestamp = mosquitto_time();
				msg->dup = 1; /* Any retry attempts are a duplicate. */
				msg->state = mosq_ms_wait_for_pubrec;
			}else if(rc == MOSQ_ERR_OVERSIZE_PACKET){
				db__message_remove(db, &context->msgs_out, msg);
			}else{
				return rc;
			}
			break;

		case mosq_ms_resend_pubrel:
			rc = send__pubrel(context, mid, NULL);
			if(!rc){
				msg->state = mosq_ms_wait_for_pubcomp;
			}else{
				return rc;
			}
			break;

		case mosq_ms_invalid:
		case mosq_ms_send_pubrec:
		case mosq_ms_resend_pubcomp:
		case mosq_ms_wait_for_puback:
		case mosq_ms_wait_for_pubrec:
		case mosq_ms_wait_for_pubrel:
		case mosq_ms_wait_for_pubcomp:
		case mosq_ms_queued:
			break;
	}
	return MOSQ_ERR_SUCCESS;
}


int db__message_write_inflight_out_all(struct mosquitto_db *db, struct mosquitto *context)
{
	struct mosquitto_client_msg *tail, *tmp;
	int rc;

	if(context->state != mosq_cs_active || context->sock == INVALID_SOCKET){
		return MOSQ_ERR_SUCCESS;
	}

	DL_FOREACH_SAFE(context->msgs_out.inflight, tail, tmp){
		rc = db__message_write_inflight_out_single(db, context, tail);
		if(rc) return rc;
	}
	return MOSQ_ERR_SUCCESS;
}

