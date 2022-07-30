# vim: ts=4 sw=4 noet
#!/bin/bash

E2MGR_ADDR=10.152.183.159:3800
RAN_NAME=""


usage() {
	echo "
Usage: $0 [-a|--add] [-l|--list] [-s|--states] [-n|--nodeb inventory_name]
"
	exit 1
}

do_list() {
	curl -s "http://${E2MGR_ADDR}/v1/e2t/list" -H "accept: application/json" | jq .
}

do_states() {
	curl -s "http://${E2MGR_ADDR}/v1/nodeb/states" -H "accept: application/json" | jq .
}

do_shownodeb() {
	curl -s "http://${E2MGR_ADDR}/v1/nodeb/${RAN_NAME}" -H "accept: application/json" | jq .
}

do_add_enb() {
	curl -X POST -H "accept: application/json" -H "Content-Type: application/json" -d @input-e2mgr.txt "http://${E2MGR_ADDR}/v1/nodeb/enb"
}



if [[ "$#" -eq 0 ]]; then
	usage
fi

while [[ "$#" -gt 0 ]]; do
	case $1 in
		-a|--add)
			do_add_enb
			shift;;
		-l|--list)
			do_list
			shift
			;;
		-s|--states)
			do_states
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

