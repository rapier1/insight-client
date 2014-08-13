/* Pseudo-Client functions and variables */

var clientCallCnt = 0;


// Temp function to simulate client sending data
function clientSend(){
	var strJSON = null;
	switch(clientCallCnt){
	case 0:
		strJSON = '{ "array" : [ ';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "64.233.171.95", "destPort" : "80", "protocol" : "tcp" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "24.193.17.132", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "68.100.103.161", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"}';
		strJSON += '] }';
		break;
	case 1:
		strJSON = '{ "array" : [ ';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "64.233.171.95", "destPort" : "80", "protocol" : "tcp" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "24.193.17.132", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "68.100.103.161", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "71.60.190.139", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "68.100.103.161", "destPort" : "8080", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"}';
		strJSON += '] }';
		break;
	case 2:
		strJSON = '{ "array" : [ ';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "64.233.171.95", "destPort" : "80", "protocol" : "tcp" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "68.100.103.161", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "71.60.190.139", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "12.3.212.0", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"}';
		strJSON += '] }';
		break;
	case 3:
		strJSON = '{ "array" : [ ';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "64.233.171.95", "destPort" : "80", "protocol" : "tcp" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "68.100.103.161", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "71.60.190.139", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "12.3.212.0", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"},';
		strJSON += '{ "tuple" : { "srcIP" : "128.182.160.117", "srcPort" : "80", "destIP" : "68.84.239.0", "destPort" : "80", "protocol" : "value" }, "priority" : "1", "dupAcks" : "0", "oops" : "0", "cwnd" : "0", "winScale" : "0"}';
		strJSON += '] }';
		break;
	case 4:
		break;
	case 5:
		break;
	}

	clientCallCnt++;
	return strJSON;
}
