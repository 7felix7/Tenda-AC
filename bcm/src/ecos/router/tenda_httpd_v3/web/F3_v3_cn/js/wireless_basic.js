var max_wds_num = 2,
	request = GetReqObj(),
	http_request2 = null,
	ChannelList_24G = [_("AutoSelect"), _("2412MHz (Channel 1)"), _("2417MHz (Channel 2)"),
			_("2422MHz (Channel 3)"), _("2427MHz (Channel 4)"), _("2432MHz (Channel 5)"),
			_("2437MHz (Channel 6)"), _("2442MHz (Channel 7)"), _("2447MHz (Channel 8)"),
			_("2452MHz (Channel 9)"), _("2457MHz (Channel 10)"), _("2462MHz (Channel 11)"),
			_("2467MHz (Channel 12)"), _("2472MHz (Channel 13)"), _("2484MHz (Channel 14)")],
	WirelessWorkMode,
	wdsList,
	PhyMode,
	broadcastssidEnable,
	channel_index,
	countrycode,
	ht_bw,
	ht_extcha,
	enable_wl,
	mode,
	wmmCapable,
	APSDCapable,
	SSID0,
	SSID1,
	wireless11bchannels,
	wireless11gchannels,
	ap_isolate,
	wl0_mode;

function showChannel() {
	var idx, bstr;
	var AutoSelectx = _("AutoSelect");
	var Channelx = _("Channel");
	if(channel_index == 0) {
		bstr = '<select id="sz11bChannel" name="sz11bChannel" size="1" onChange="ChannelOnChange()"><option value=0 selected>' + AutoSelectx + '</option>';
	} else {
		bstr = '<select id="sz11bChannel" name="sz11bChannel" size="1" onChange="ChannelOnChange()"><option value=0>' + AutoSelectx + '</option>';
	}
	if(wireless11bchannels == 11) {
		for (idx = 0; idx < 11; idx++) {
			if(channel_index==idx  + 1) {
				bstr+='<option value='+(idx+1)+' selected>'+(2412+5*idx)+'MHz (' + Channelx + ' '+(idx+1)+')</option>';
			} else {
				bstr+='<option value='+(idx+1)+'>'+(2412+5*idx)+'MHz (' + Channelx+ ' '+(idx+1)+')</option>';
			}
		}
	} else if(wireless11bchannels == 13) {
		for (idx = 0; idx < 13; idx++) {
			if(channel_index==idx  + 1) {
				bstr+='<option value='+(idx+1)+' selected>'+(2412+5*idx)+ 'MHz (' + Channelx+ ' '+(idx+1)+')</option>';
			} else {
				bstr+='<option value='+(idx+1)+'>'+(2412+5*idx)+ 'MHz (' + Channelx+ ' '+(idx+1)+')</option>';
			}
		}
	} else if (wireless11bchannels == 14) {
		for(idx = 0; idx < 13; idx++) {
			if(channel_index ==idx + 1) {
				bstr+='<option value='+(idx+1)+' selected>'+(2412+5*idx)+ 'MHz (' + Channelx+ ' '+(idx+1)+')</option>';
			} else {
				bstr+='<option value='+(idx+1)+'>'+(2412+5*idx)+ 'MHz (' + Channelx+ ' '+(idx+1)+')</option>';
			}
		}
		if(channel_index==14) {
			bstr=+'<option value=14 selected>2484MHz (' + Channelx + ' 14)</option>';
		} else {
			bstr=+'<option value=14>2484MHz (' + Channelx + ' 14)</option>';
		}
	}
	bstr+='</select>';
	document.getElementById("11b").innerHTML=bstr;
}

//++++++++++++
function initWds() {
	var wdslistArray;
	if (wdsList != "") {
		wdslistArray = wdsList.split(" ");
		for(i = 1; i <= wdslistArray.length; i++) {
			document.wireless_basic["wds_" + i].value = wdslistArray[i - 1];
		}
	}
	document.getElementById("wdsScanTab").style.display = "none";
	document.getElementById("wlSurveyBtn").value = _("Open Scan");
}

function closeSurvey(){
	var tbl = document.getElementById("wdsScanTab").style,
		code = "/goform/WDSScan";	
	if (tbl.display == "") {
		tbl.display = "none";
		document.getElementById("wlSurveyBtn").value= _("Open Scan");
	} else {
		document.body.className = "onwdsscan";
		tbl.display="";
		document.getElementById("wlSurveyBtn").value= _("Close Scan");
		request.open("GET", code, true);
		request.onreadystatechange = RequestRes;
		request.setRequestHeader("If-Modified-Since","0");
		request.send(null);
	}	
}

function RequestRes(){
	if (request.readyState == 4) {
		initScan(request.responseText);
		
		//reset this iframe height by call parent iframe function
		window.parent.reinitIframe();
	}
}
function initScan(scanInfo) {
	var str1, str, len, tbl, maxcell, nc, nr, j, i;		
	/* 网页超时后点无线信号搜索时出现乱码 */	
	if(scanInfo.indexOf("<!DOCTYPE ") != "-1") {
		top.location.reload(true);
		return;	
	}	
	if (scanInfo != '') {
	    var iserror=scanInfo.split("\n");
		if(iserror[0]!="stanley") {
			str1 = scanInfo.split("\r");
			len = str1.length;
			document.getElementById("wdsScanTab").style.display = "";
			document.getElementById("wlSurveyBtn").value = _("Close Scan");			
			tbl = document.getElementById("wdsScanTab").getElementsByTagName('tbody')[0];	
			maxcell = tbl.rows.length;
			for (j = maxcell; j > 1; j --) {
				tbl.deleteRow(j - 1);
			}
			for (i = 0; i < len; i ++) {
				str = str1[i].split("\t");
				nr = document.createElement('tr');
				nc = document.createElement('td');
				nr.appendChild(nc);	
				nc.innerHTML = "<input type=\"radio\" name=\"wlsSlt\" value=\"radiobutton\" onclick=\"macAcc()\"/>";				
				nc=document.createElement('td');
				nr.appendChild(nc);
				nc.className = "fixed";
				nc.title = decodeSSID(str[0]);
				nc.innerHTML = str[0];				
				nc=document.createElement('td');
				nr.appendChild(nc);
				nc.className = "fixed";
				nc.title = str[1];
				nc.innerHTML = str[1];			
				nc=document.createElement('td');
				nr.appendChild(nc);
				nc.className = "fixed";
				nc.title = str[2];
				nc.innerHTML = str[2];				
				nc=document.createElement('td');
				nr.appendChild(nc);
				nc.className = "fixed";
				nc.title = str[3];
				nc.innerHTML = str[3];	
				nc = document.createElement('td');
				nr.appendChild(nc);
				nc.className = "fixed";
				nc.title = str[4];
				nc.innerHTML = str[4];				
				nr.align = "center";
				tbl.appendChild(nr);
			}
		}
		document.body.className = "";
	} else {
		document.getElementById("wdsScanTab").style.display = "none";
		document.getElementById("wlSurveyBtn").value=_("Open Scan");
	}
}

function macAcc() {
	if(!confirm(_("Please click on OK to confirm that you want to connect to this AP!"))) {
		return ;
	}
	var tbl = document.getElementById("wdsScanTab");
	var doc = document.wireless_basic;
	var mac, sc, ssid, schannel;
	var maxcell = tbl.rows.length;	
	for (var r = maxcell; r > 1; r --) {
		if (maxcell == 2) {
			sc = doc.wlsSlt.checked;
		} else {
			sc = doc.wlsSlt[r - 2].checked;
		}
		if (sc) {
			mac = tbl.rows[r - 1].cells[2].innerHTML;
			ssid = tbl.rows[r - 1].cells[1].innerHTML;
			schannel = tbl.rows[r - 1].cells[3].innerHTML;
			if(checkRepeat((mac)) == false) {
				return;
			}
			for(var k=1;k<=max_wds_num;k++) {   
				if(doc.elements["wds_"+k].value == "") {
					doc.elements["wds_"+k].value = mac;
					doc.elements["ssid_"+k].value = decodeSSID(ssid);
					doc.elements["schannel_"+k].value = schannel;
					//把SSID 和信道改成和对方一样
					doc.elements["ssid"].value = decodeSSID(ssid);
					doc.elements["sz11bChannel"].value = schannel;
					ChannelOnChange();					
					return true;
				}
				if(k == max_wds_num) {
					alert(_("AP MAC enteries are full! Please remove the MAC enteries you don't need."));
					return ;
				}
			}
		}
	}
}

function checkRepeat(mac){
	for(var i=1;i<=max_wds_num;i++) {
		if(mac.toUpperCase() == document.forms[0].elements["wds_"+i].value.toUpperCase()) {
			alert(_("This address already exists!"));
			return false;
		}
	}
}
function onenablewirelesschange() {
	if(document.getElementById("enablewireless").checked) {
		document.getElementById("divwieless").style.display="";
		
		//reset this iframe height by call parent iframe function
		window.parent.reinitIframe();
	} else {
		document.getElementById("divwieless").style.display="none";
	}
	
}

function insertExtChannelOption() {
	var wmode = document.wireless_basic.wirelessmode.options.selectedIndex,
		channel_index=Number(document.getElementById("sz11bChannel").selectedIndex);
	if ((wmode == 3)) { //11bgn
		var x = document.getElementById("n_extcha");
		var leng = document.getElementById("sz11bChannel").length;
		if(channel_index == 0){
			x.length=1;
			x.options[0].text = _("AutoSelect");
			x.options[0].value = 0;
		} else if((channel_index >=1) && (channel_index <= 4)) {
			x.length=1;
			x.options[0].text = ChannelList_24G[channel_index+4];
			x.options[0].value = 1;
		} else if ((channel_index >= 5) && (channel_index <= 7)) {
			x.length=2;
			x.options[0].text = ChannelList_24G[channel_index-4];
			x.options[0].value = 0;
			x.options[1].text = ChannelList_24G[channel_index+4];
			x.options[1].value = 1;
		} else if (channel_index>7 && channel_index<10) {
			if(leng>=14) {
				x.length=2;
				
				x.options[0].text = ChannelList_24G[channel_index-4];
				x.options[0].value = 0;
				x.options[1].text = ChannelList_24G[channel_index+4];
				x.options[1].value = 1;
			} else {
				x.length=1;
				x.options[0].text = ChannelList_24G[channel_index-4];
				x.options[0].value = 0;
			}
		} else {
			if(channel_index==10 && leng==15) {
				x.length=2;
				
				x.options[0].text = ChannelList_24G[channel_index-4];
				x.options[0].value = 0;
				x.options[1].text = ChannelList_24G[channel_index+4];
				x.options[1].value = 1;
			} else {
				x.length=1;
				x.options[0].text = ChannelList_24G[channel_index-4];
				x.options[0].value = 0;
			}
		}
	}
}

function ChannelOnChange() {
	if(document.wireless_basic.wirelessmode.options.selectedIndex==3) {
		Channel_BandWidth_onClick()
		insertExtChannelOption();
	}
}

function Channel_BandWidth_onClick() {
	if (document.wireless_basic.n_bandwidth[0].checked == true) {
		document.wireless_basic.n_extcha.disabled = true;
	} else {
		document.wireless_basic.n_extcha.disabled = false;
	}
}
function initValue() {
	var ssidArray,
		current_channel_length,
		ap_isolateArray,
		doc = document.wireless_basic;

    document.getElementById("ssid").value = decodeSSID(SSID0);
	document.getElementById("mssid_1").value = decodeSSID(SSID1);
	PhyMode = 1*PhyMode;
	if ((PhyMode == 0) || (PhyMode == 1) || (PhyMode == 4) || (PhyMode == 9)) {
		if (PhyMode == 0) {
			doc.wirelessmode.options.selectedIndex = 0;
		} else if (PhyMode == 1) {
			doc.wirelessmode.options.selectedIndex = 1;
		} else if (PhyMode == 4) {
			doc.wirelessmode.options.selectedIndex = 2;
		} else if (PhyMode == 9) {
			doc.wirelessmode.options.selectedIndex = 3;
			document.getElementById("div_11n").style.display = "";
			doc.n_bandwidth.disabled = false;
		}
	}
	if (wmmCapable == "on") {
		doc.wmm_capable[0].checked = true;
		document.getElementById("div_apsd_capable").style.display = "";
		//doc.apsd_capable.disabled = false;
	} else {
		doc.wmm_capable[1].checked = true;
		document.getElementById("div_apsd_capable").style.display = "none";
		//doc.apsd_capable.disabled = true;
	}
	if (APSDCapable == "on") {
		doc.apsd_capable[0].checked = true;
	} else {
		doc.apsd_capable[1].checked = true;
	}
	if (broadcastssidEnable == 0) {
		doc.broadcastssid[0].checked = true;
	} else {
		doc.broadcastssid[1].checked = true;
	}
	ap_isolateArray = ap_isolate.split(";");
	if (1*ap_isolateArray[0] == 0) {
		doc.ap_isolate[1].checked = true;
	} else {
		doc.ap_isolate[0].checked = true;
	}
	if (ht_bw == 0) {
		doc.n_bandwidth[0].checked = true;
		doc.n_extcha.disabled = true;
	} else if(ht_bw == 1){
		doc.n_bandwidth[1].checked = true;
		doc.n_extcha.disabled = false;
	} else {
		doc.n_bandwidth[2].checked = true;
		doc.n_extcha.disabled = false;
	}
	insertExtChannelOption();
	if(ht_extcha == 'upper') {
		doc.n_extcha.options.selectedIndex = 0;
	} else if(ht_extcha == 'lower' ) {
		if(channel_index>=1&&channel_index<=4) {
			doc.n_extcha.options.selectedIndex = 0;
		} else {
			doc.n_extcha.options.selectedIndex = 1;
		}
	} else {
		doc.n_extcha.options.selectedIndex = 0;
	}
}
function changeWds(x){
	if(x==0) {
		document.getElementById("wdsMode").style.display = "none";
	} else {
		document.getElementById("wdsMode").style.display = "";
	}
	
	//reset this iframe height by call parent iframe function
	window.parent.reinitIframe();
}
function wmm_capable_enable_switch(){
	if (document.wireless_basic.wmm_capable[0].checked == true) {
		document.getElementById("div_apsd_capable").style.display = "";
		document.wireless_basic.apsd_capable.disabled = false;
	} else {
		document.getElementById("div_apsd_capable").style.display = "none";
		document.wireless_basic.apsd_capable.disabled = true;
	}
}
function wirelessModeChange() {
	var wmode=document.wireless_basic.wirelessmode.options.selectedIndex;
	document.getElementById("div_11n").style.display = "none";
	document.wireless_basic.n_bandwidth.disabled = true;
	wmode = 1*wmode;
	if (wmode == 3) {
		document.getElementById("div_11n").style.display = "";
		document.wireless_basic.n_bandwidth.disabled = false;
		insertExtChannelOption();
	} else {
		document.getElementById("div_11n").style.display = "none";
		document.wireless_basic.n_bandwidth.disabled = true;
	}
	if(mode==1) {
		document.getElementById("sz11bChannel").disabled=true;
	}
}
function CheckValue() {
	var submit_ssid_num,
		doc =document.wireless_basic,
		//reSsid = /^[\w`~!@#$^*()\-+={}\[\]\|:'<>.?\/ ]+$/,
		sid = doc.ssid.value,
		sid1 = doc.mssid_1.value,
		all_wds_list = '',
		reMac = /([A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}/,
		macI,
		macJ;
		
	if(!checkSSID(sid)) {
		//alert(_("SSID can not contain comma, semicolon, double quotation marks, ampersand, percent and backslash!"));
		doc.ssid.focus();
		doc.ssid.select();
		return false;
	}
	if(wl0_mode != "ap") {
		if(!checkSSID(sid1)){
			//alert(_("SSID can not contain comma, semicolon, double quotation marks, ampersand, percent and backslash!"))
			doc.mssid_1.focus();
			doc.mssid_1.select();
			return false;
		}
	} else {
		if(sid1 != "" && !checkSSID(sid1)){
			//alert(_("SSID can not contain comma, semicolon, double quotation marks, ampersand, percent and backslash!"))
			doc.mssid_1.focus();
			doc.mssid_1.select();
			return false;
		}
	}
	
	if(sid != decodeSSID(SSID0)){
		var con=confirm(_("The SSID (network name) will be changed to %s! Please reconnect to the new SSID!",[sid]));
		if(con == false){
			doc.ssid.value=decodeSSID(SSID0);
			return false;
		}
	}
	if(sid1 != decodeSSID(SSID1)) {
		if(sid1 == "") {
			var con=confirm(_("Secondary SSID is disabled! Devices connected to it will be unable to access the wireless network!"));
		} else {
			var con=confirm(_("Secondary SSID has been changed to %s! Please reconnect using the new SSID!", [sid1]));
		}
		if(con == false){
			doc.mssid_1.value=decodeSSID(SSID1);
			return false;
		}
	}
	submit_ssid_num = 1;
	doc.bssid_num.value = submit_ssid_num;
	if (doc.WirelessT[1].checked==true) {
		
		for (i = 1; i <= max_wds_num; i++) {
			macI = document.wireless_basic["wds_" + i].value;
			
			if (macI === "") {
				continue;
			}
			if (macI === "00:00:00:00:00:00"){
					alert(_("Please enter a valid MAC address!"));
					return false;
			}
			if((macI.charAt(1) == "1") || (macI.charAt(1) == "3") ||
					(macI.charAt(1) == "5") || (macI.charAt(1) == "7") ||
					(macI.charAt(1) == "9") || (macI.charAt(1).toLowerCase() == "b")|| 
					(macI.charAt(1).toLowerCase() == "d") ||
					(macI.charAt(1).toLowerCase() == "f")){
      		  	alert(_("Please enter the MAC address of your wireless device or correct unicast MAC address!"));
       		 	return false;
   			}
		 
			for (j = 1; j < i; j++) {
				macJ = document.wireless_basic["wds_" + j].value;
				
				if(macI === macJ ){
					alert(_("Please enter a different MAC address."));
					return false;
				}
			}
			if (!reMac.test(macI)){
					alert(_("Please enter a valid MAC address!"));
					return false;
			} else {
				all_wds_list += macI;
				all_wds_list += ' ';
			}
		}
		if (all_wds_list == "") {
			alert(_("AP MAC address field should not be left empty!"));
			doc.wds_1.focus();
			doc.wds_1.select(); 
			return false;
		} else {
			doc.wds_list.value = all_wds_list;
		}
	}
	return true;
}

function preSubmit(f){  
	var sid = f.ssid.value,
		sid1 = f.mssid_1.value,
		fm = document.forms[0];
	if(fm.elements["enablewireless"].checked) {
		fm.elements["enablewirelessEx"].value = "1";
	} else {
		fm.elements["enablewirelessEx"].value = "0";
	}		
	if (CheckValue()) { 
		f.submit();
		if(sid == decodeSSID(SSID0) && sid1 == decodeSSID(SSID1)){
			showSaveMassage();
		}
	}
}
function init(){
	var wireContent = document.getElementById("divwieless"),
		wireEnable = document.getElementById("enablewireless"),
		WirelessTs = document.wireless_basic.WirelessT;
		
    if(WirelessWorkMode==0){
		WirelessTs[0].checked = true;
		changeWds(0);
	} else if (WirelessWorkMode==1) {
		WirelessTs[1].checked = true;
		changeWds(1);	 
	}
	initWds();
	if(enable_wl==1){
		wireEnable.checked=true;
    	wireContent.style.display="";
	} else {
		wireEnable.checked=false;
		wireContent.style.display="none";		
	}
	initValue();
	if(mode==1 || wl0_mode != "ap"){
		document.getElementById("sz11bChannel").disabled=true;
		//document.getElementById("sz11gChannel").disabled=true;
		document.getElementById("enablewireless").disabled=true;
	}
	if(wl0_mode != "ap"){
		//当开启apclient时，禁用wds等
		WirelessTs[1].disabled = true;
		wireEnable.disabled=true;
		document.wireless_basic.broadcastssid[1].disabled = true;
		document.wireless_basic.ssid.disabled = true;
	}
}

function alertContents2() {
	var str;
	if (http_request2.readyState == 4) {
		if (http_request2.status == 200) {
		 	str = http_request2.responseText.split("\r");
			WirelessWorkMode = str[0];//0
			wdsList = str[1];//
			PhyMode = str[2];//9
			broadcastssidEnable = str[3];//0
			channel_index = str[4];//0
			countrycode = str[5];//US
			ht_bw = str[6];//1
			ht_extcha = str[7];//none
			enable_wl = str[8];//1
			mode = str[9];//ap
			wmmCapable = str[10];//on
			APSDCapable = str[11];//off
			SSID0 = ssid000; //str[12];
			SSID1 = ssid111;//str[13];
			wireless11bchannels = str[14];//11
			wireless11gchannels = str[15];//11
			ap_isolate = str[16];
			wl0_mode = str[17];
			showChannel();
			init();
		}
	}
}

function initbasic() {
	http_request2 = GetReqObj();
	http_request2.onreadystatechange = alertContents2;
	http_request2.open('POST', "/goform/wirelessInitBasic", true);
	http_request2.send("something");
}

window.onload = initbasic;