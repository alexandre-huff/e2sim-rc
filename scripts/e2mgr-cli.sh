# vim: ts=4 sw=4 noet
#!/bin/bash

#E2MGR_IP=$(kubectl get nodes -o jsonpath='{.items[0].status.addresses[0].address}')
#E2MGR_PORT=$(kubectl -n ricplt get service service-ricplt-e2mgr-http -o jsonpath='{.spec.ports[0].nodePort}')
E2MGR_IP=$(kubectl -n ricplt get po -l release=r4-e2mgr -o jsonpath='{.items[0].status.podIP}')
E2MGR_PORT=$(kubectl -n ricplt get service service-ricplt-e2mgr-http -o jsonpath='{.spec.ports[0].port}')
E2MGR_ADDR=${E2MGR_IP}:${E2MGR_PORT}
RAN_NAME=""


usage() {
	echo "
Usage: $0 [-a|--add] [-l|--list] [-s|--states] [-n|--nodeb inventory_name] [d|--del inventory_name]
"
	exit 1
}

do_list() {
	curl -s "http://${E2MGR_ADDR}/v1/e2t/list" -H "accept: application/json" | jq .
}

do_states() {
	curl -s "http://${E2MGR_ADDR}/v1/nodeb/states" -H "accept: application/json" | jq .
}

do_symptomdata() {
	curl -s "http://${E2MGR_ADDR}/v1/nodeb/symptomdata" -H "accept: application/json" | jq .
}

do_shownodeb() {
	curl -s "http://${E2MGR_ADDR}/v1/nodeb/${RAN_NAME}" -H "accept: application/json" | jq .
}

do_add_enb() {
	curl -X POST -H "accept: application/json" -H "Content-Type: application/json" -d @input-e2mgr.txt "http://${E2MGR_ADDR}/v1/nodeb/enb"
}

do_del_enb() {
	curl -v -X DELETE "http://${E2MGR_ADDR}/v1/nodeb/gnb/${RAN_NAME}" -H "accept: application/json" -H "Content-Type: application/json"
}

do_shutdown() {
	curl -v -X PUT "http://${E2MGR_ADDR}/v1/nodeb/shutdown" -H "accept: application/json" -H "Content-Type: application/json"
}


if [[ "$#" -eq 0 ]]; then
	usage
fi

while [[ "$#" -gt 0 ]]; do
	case $1 in
		-a|--add)
			do_add_enb
			shift;;
		-d|--del)
			shift
			RAN_NAME=$1
			do_del_enb
			shift;;
		--shutdown)
			do_shutdown
			shift
			;;
		-l|--list)
			do_list
			shift
			;;
		-s|--states)
			do_states
			shift
			;;
		--symptomdata)
			do_symptomdata
			shift
			;;
		-n|--nodeb)
			shift
			RAN_NAME=$1
			do_shownodeb
			shift
			;;
		*)
			usage
			;;
	esac;
	shift;
done

