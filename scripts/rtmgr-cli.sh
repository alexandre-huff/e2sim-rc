# vim: ts=4 sw=4 noet
#!/bin/bash

RT_MGR_IP=10.152.183.194

usage() {
	echo "
Usage: $0 [-a|--addrmrroute] [-d|--delrmrroute] [-t|--routingtable]
"
	exit 1
}

do_addrmrroute() {
	curl -v -X POST "http://${RT_MGR_IP}:3800/ric/v1/handles/addrmrroute" -H "accept: application/json" -H "Content-Type: application/json" -d "@routes.json"
}

do_delrmrroute() {
	curl -v -X DELETE "http://${RT_MGR_IP}:3800/ric/v1/handles/delrmrroute" -H "accept: application/json" -H "Content-Type: application/json" -d "@routes.json"
}

do_debuginfo() {
	curl "http://${RT_MGR_IP}:3800/ric/v1/getdebuginfo" -H "accept: application/json" | jq .
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

