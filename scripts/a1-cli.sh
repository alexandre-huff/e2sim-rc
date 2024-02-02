# vim: ts=4 sw=4 noet
#!/bin/bash

# Author: Alexandre Huff

#set -x

usage() {
        echo "
Usage: $0 
	[-c|--create_policy_type POLICY_TYPE_ID JSON_FILE]
	[-p|--create_policy_instance POLICY_TYPE_ID POLICY_INSTANCE_ID JSON_FILE]
	[-g|--get_policy_types]
	[   --get_policy_type POLICY_TYPE_ID]
	[-i|--get_policy_instances POLICY_TYPE_ID]
	[   --get_policy_instance POLICY_TYPE_ID POLICY_INSTANCE_ID]
	[-s|--get_policy_instance_status POLICY_TYPE_ID POLICY_INSTANCE_ID]
	[   --delete_policy_type POLICY_TYPE_ID]
	[   --delete_policy_instance POLICY_TYPE_ID POLICY_INSTANCE_ID]
	[   --data-delivery JSON_FILE]
"
        exit 1
}
                		

if [[ "$#" -eq 0 ]]; then
        usage
fi


A1_IP=$(kubectl -n ricplt get po -l release=r4-a1mediator -o jsonpath='{.items[0].status.podIP}')
A1_PORT=$(kubectl -n ricplt get service service-ricplt-a1mediator-http -o jsonpath='{.spec.ports[0].port}')
A1_ADDR=${A1_IP}:${A1_PORT}


do_create_policy_type() {
	curl -v -X PUT "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}" -H "Accept: application/json" -H "Content-Type: application/json" -d @${JSON_FILE}
}

do_create_policy_instance() {
	curl -v -X PUT "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}/policies/${POLICY_INSTANCE_ID}" -H "Accept: application/json" -H "Content-Type: application/json" -d @${JSON_FILE}
}

do_get_policy_types() {
	curl -s GET "http://${A1_ADDR}/a1-p/policytypes/" -H "Accept: application/json"
}

do_get_policy_type() {
	curl -s GET "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}" -H "Accept: application/json" | jq .
}

do_get_policy_instances() {
	curl -s GET "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}/policies/" -H "Accept: application/json" | jq .
}

do_get_policy_instance() {
	curl -s GET "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}/policies/${POLICY_INSTANCE_ID}" -H "Accept: application/json" | jq .
}

do_get_policy_instance_status() {
	curl -s GET "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}/policies/${POLICY_INSTANCE_ID}/status" -H "Accept: application/json" | jq .
}

do_delete_policy_type() {
	curl -v -X DELETE "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}/" -H "Accept: application/json"
}

do_delete_policy_instance() {
	curl -v -X DELETE "http://${A1_ADDR}/a1-p/policytypes/${POLICY_TYPE_ID}/policies/${POLICY_INSTANCE_ID}/" -H "Accept: application/json"
}

do_data_delivery() {
	curl -X POST "http://${A1_ADDR}/data-delivery" -H "Content-Type: application/json" -d @${JSON_FILE}
}


while [[ "$#" -gt 0 ]]; do
        case $1 in
                -c|--create_policy_type)
                		shift
                		POLICY_TYPE_ID=$1
                		JSON_FILE=$2
                        do_create_policy_type
                        shift 2
                        ;;
                -p|--create_policy_instance)
                        shift
                        POLICY_TYPE_ID=$1
                        POLICY_INSTANCE_ID=$2
                		JSON_FILE=$3
                        do_create_policy_instance
                        shift 3
                        ;;
                -g|--get_policy_types)
                        do_get_policy_types
                        shift
                        ;;
                --get_policy_type)
                		shift
                		POLICY_TYPE_ID=$1
                        do_get_policy_type
                        shift
                        ;;
                -i|--get_policy_instances)
                		shift
                		POLICY_TYPE_ID=$1
                        do_get_policy_instances
                        shift
                        ;;
                --get_policy_instance)
                		shift
                		POLICY_TYPE_ID=$1
                		POLICY_INSTANCE_ID=$2
                        do_get_policy_instance
                        shift 2
                        ;;
                -s|--get_policy_instance_status)
                        shift
                        POLICY_TYPE_ID=$1
                		POLICY_INSTANCE_ID=$2
                        do_get_policy_instance_status
                        shift 2
                        ;;
                --delete_policy_type)
                        shift
                        POLICY_TYPE_ID=$1
                        do_delete_policy_type
                        shift
                        ;;
                --delete_policy_instance)
                        shift
                        POLICY_TYPE_ID=$1
                	POLICY_INSTANCE_ID=$2
                        do_delete_policy_instance
                        shift 2
                        ;;
		--data_delivery)
			shift
			JSON_FILE=$1
			do_data_delivery
			shift
			;;
                *)
                        usage
                        ;;
        esac;
        shift;
done

