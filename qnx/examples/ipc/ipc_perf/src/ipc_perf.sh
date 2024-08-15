LAST_RPROC_ID=$1
START_RPROC_ID=$2
CUR_RPROC=$START_RPROC_ID
echo "# IPC datasheet for `uname -sm` {#ipc_datasheet}"
echo ""
echo ""
echo "# Average Round Trip Time in microseconds"
echo ""
echo "The average of 10K round trip times, in microseconds, for a 9-byte (ping)"
echo "message sent from A72 to all remote cores in the SoC that support an echo test"
echo ""
echo ""

echo "RPROC       | AVG RTT (usecs)  "
echo "------------|------------"
while [ $CUR_RPROC -le $LAST_RPROC_ID ]; do
	ipc_perf -s -c$CUR_RPROC -n10000 | grep "\|"
	let CUR_RPROC=CUR_RPROC+1
done

echo ""
