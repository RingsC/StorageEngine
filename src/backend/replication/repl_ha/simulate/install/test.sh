#/bin/sh

ulimit -c 64

cmd="python test_simulate.py ./simulate_env data 10 40"
ntests=0
while [ $ntests -lt 20 ]
do
	echo "=================="$ntests" test START==============="
	$cmd

	ret=$?
	if [ $ret -ne 0 ]; then
		echo "=================="$ntests" test FAILED=============="
		exit 1
	fi
	echo "================"$ntests" test SUCCESSED============="

	ntests=`expr $ntests + 1`
done
