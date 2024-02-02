# vim: ts=4 sw=4 noet
#!/bin/bash

#RTMGR_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[0].address}')
#RTMGR_PORT=$(kubectl -n ricplt get service service-ricplt-rtmgr-http -o jsonpath='{.spec.ports[0].nodePort}')
RTMGR_IP=$(kubectl -n ricplt get po -l release=r4-rtmgr -o jsonpath='{.items[0].status.podIP}')
RTMGR_PORT=$(kubectl -n ricplt get service service-ricplt-rtmgr-http -o jsonpath='{.spec.ports[0].port}')
RTMGR_ADDR=${RTMGR_IP}:${RTMGR_PORT}
E2T_ADDR=""
RAN_NAME=""

usage() {
	echo "
Usage: $0 [-a|--addrmrroute] [-d|--delrmrroute] [-t|--routingtable] [--del-e2t <E2TAddress>] [--dissociate-ran <E2TAddress> <RAN_NAME>]
"
	exit 1
}

do_addrmrroute() {
	curl -v -X POST "http://${RTMGR_ADDR}/ric/v1/handles/addrmrroute" -H "accept: application/json" -H "Content-Type: application/json" -d "@routes.json"
}

do_delrmrroute() {
	curl -v -X DELETE "http://${RTMGR_ADDR}/ric/v1/handles/delrmrroute" -H "accept: application/json" -H "Content-Type: application/json" -d "@routes.json"
}

do_debuginfo() {
	curl -s "http://${RTMGR_ADDR}/ric/v1/getdebuginfo" -H "accept: application/json" | jq .
}

do_del_e2t() {
	if [ -z ${E2T_ADDR} ]; then
		usage
		return
	fi
    curl -v -X DELETE "http://${RTMGR_ADDR}/ric/v1/handles/e2t" -H "accept: application/json" -H "Content-Type: application/json" -d "{ \"E2TAddress\": \"${E2T_ADDR}\", \"ranNamelist\": [] }"
}

do_dissociate_ran() {
	if [ -z "${E2T_ADDR}" ] || [ -z "${RAN_NAME}" ]; then
		usage
		return
	fi
	curl -v -X POST "http://${RTMGR_ADDR}/ric/v1/handles/dissociate-ran" -H "accept: application/json" -H "Content-Type: application/json" -d "[ { \"E2TAddress\": \"${E2T_ADDR}\", \"ranNamelist\": [ \"${RAN_NAME}\" ] }]"
}

if [[ "$#" -eq 0 ]]; then
	usage
fi

while [[ "$#" -gt 0 ]]; do
	case $1 in
		-a|--addrmrroute)
			do_addrmrroute
			shift
			;;
		-d|--delrmrroute)
			do_delrmrroute
			shift
			;;
		--del-e2t)
			shift
			E2T_ADDR=$1
			do_del_e2t
			shift
			;;
		--dissociate-ran)
			shift
			E2T_ADDR=$1
			shift
			RAN_NAME=$1
			do_dissociate_ran
			shift
			;;
		-t|--routingtable)
			do_debuginfo
			shift
			;;
		*)
			usage
			;;
	esac;
	shift;
done

