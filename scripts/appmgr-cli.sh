# vim: ts=4 sw=4 noet
#!/bin/bash

# set -e -o pipefail

usage() {
	echo "
Usage: $0 [[-r|--register] [-d|--deregister] {config-file.json}] [-l|--list]
"
	exit 1
}


if [[ "$#" -lt 1 || "$#" -gt 2 ]]; then
	usage
fi


APP_MGR_IP=10.152.183.79
OPT=$1
CONFIG_FILE=$2



do_set_xapp_name() {
	if [[ -n ${CONFIG_FILE} && -f ${CONFIG_FILE} ]]; then
		XAPP_NAME=$(jq '.xapp_name' < ${CONFIG_FILE})
		XAPP_NAME=${XAPP_NAME//\"} # removing quotations
	else
		echo xapp descriptor file is required
		usage
		exit 1
	fi
	if [[ -z ${XAPP_NAME} ]]; then
		echo unable to find xapp_name in descriptor file
		exit 1
	fi
}

do_register() {
	do_set_xapp_name
	CONFIG=$(jq @json < ${CONFIG_FILE})
	VERSION=$(jq '.version' < ${CONFIG_FILE})
	ADDR=$(kubectl -n ricxapp get svc -o jsonpath="{.spec.clusterIP}" service-ricxapp-${XAPP_NAME}-rmr)
	RMR_PORT=$(kubectl -n ricxapp get svc -o jsonpath="{.spec.ports[0].port}" service-ricxapp-${XAPP_NAME}-rmr)
	HTTP_PORT=$(kubectl -n ricxapp get svc -o jsonpath="{.spec.ports[0].port}" service-ricxapp-${XAPP_NAME}-http)
	if [[ -z ${ADDR} ]]; then
		echo unable to get cluster-ip address from kubernetes
		exit 1
	fi
	RMR_ENDPT="${ADDR}:${RMR_PORT}"
	if [[ -n ${HTTP_PORT} ]]; then
		HTTP_ENDPT="${ADDR}:${HTTP_PORT}"
	fi

	cat <<EOF > ${XAPP_NAME}-register.json
{
  "appName": "${XAPP_NAME}",
  "appVersion": ${VERSION},
  "configPath": "",
  "appInstanceName": "${XAPP_NAME}",
  "httpEndpoint": "${HTTP_ENDPT}",
  "rmrEndpoint": "${RMR_ENDPT}",
  "config": ${CONFIG}
}
EOF

	# POST
	curl -v "http://${APP_MGR_IP}:8080/ric/v1/register" -H "accept: application/json" -H "Content-Type: application/json" -d "@${XAPP_NAME}-register.json"
}

do_deregister() {
	do_set_xapp_name
	# POST
	curl "http://${APP_MGR_IP}:8080/ric/v1/deregister" -H "accept: application/json" -H "Content-Type: application/json" -d "{\"appName\": \"${XAPP_NAME}\", \"appInstanceName\": \"${XAPP_NAME}\"}"
}

do_list() {
	# GET
	curl -v "http://${APP_MGR_IP}:8080/ric/v1/xapps/list" -H "accept: application/json" -H "Content-Type: application/json"
}


#while [[ "$#" -gt 0 ]]; do
case $OPT in
	-r|--register)
		do_register
#		shift
		;;
	-d|--deregister)
		do_deregister
#		shift
		;;
	-l|--list)
		do_list
#		shift
		;;
	*)
		usage
		;;
esac;
#shift;
#done

