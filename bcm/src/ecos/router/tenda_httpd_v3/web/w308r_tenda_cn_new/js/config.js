﻿var MOD = {};

/* 高级设置 */
MOD["advance"] = {
	id: 'm-1',
	text: "高级设置",
	menus: [{
			id: 'm-1-1',
			text: "运行状态",
			url: "system_status.asp",
			help: '<p>如需返回首页，请点击返回首页' +
				'<p>如需修改上网方式及相关参数，请点击上网设置。' +
				'<p class="sina-weibo"><em>腾达新浪官方微博:</em><a href="http://e.weibo.com/100tenda" target="_blank" title="腾达新浪官方微博"></a>'
		}, {
			id: 'm-1-2',
			text: "上网设置",
			url: "wan_connected.asp",
			help: '<p><span class="text-red">如何判断上网方式？</span> ' +
				'<p><em>ADSL拨号：</em>不接路由器时，需在电脑上点击宽带连接，并且输入用户名和密码才能上网。' +
				'<p><em>静态IP：</em>您的宽带运营商提供的是固定IP。' +
				'<p><em>自动获取：</em>不接路由器时，电脑不需要进行任何设置就可以直接上网。' +
				'<p>如果您不确定是哪种上网方式，建议查看您的宽带业务单据或咨询您的宽带运营商。'
		}, {
			id: 'm-1-3',
			text: "MAC克隆",
			url: 'mac_clone.asp',
			help: '<p><span class="text-red">关于MAC克隆的说明</span> ' +
				'<p>部分宽带运营商可能会将宽带账户信息与您的电脑MAC地址绑定，此时您需要进行MAC地址克隆才能正常上网。' +
				'<p><em>自动克隆：</em>如果您的宽带运营商绑定了您的电脑MAC地址，建议您用单机可以正常上网的电脑，通过网线连接到路由器，在首页或本页面点击确定即可自动克隆正确的MAC地址。' +
				'<p><em>手动克隆：</em>您可以选择恢复无线路由器出厂时默认的MAC地址，或手动填写正确的MAC地址。选择手动克隆时，克隆的MAC地址将保持不变。' +
				'<p>无线路由器恢复出厂设置后会还原为出厂时的MAC地址。'
		}, {
			id: 'm-1-4',
			text: "WAN速率控制",
			url: "wan_speed.asp",
			help: '<p>选择WAN口速率。'
		}, {
			id: 'm-1-5',
			text: "WAN口介质类型",
			url: "wireless_mode.asp",
			help: '<p><em>有线WAN模式：</em>路由器设备的接入方式由有线WAN口接入；' +
				'<p><em>无线WAN模式：</em>接入方式为无线接入，需要输入上级无线设备的无线信号名称(SSID)、信道以及安全模式等相关参数。' +
				'<p>另外，接入上网方式，请在上网设置中设置。'
		}, {
			id: 'm-1-6',
			text: "LAN口设置",
			url: "lan.asp",
			help: '<p>设置路由器的LAN IP地址和子网掩码。默认IP地址是 192.168.0.1，默认子网掩码是 255.255.255.0。'
		}, {
			id: 'm-1-7',
			text: "DNS设置",
			url: "wan_dns.asp",
			help: '<p>域名服务设置选中时，可输入您指定的域名服务器IP,并通过此域名服务器IP提供域名解析服务，未选中时宽带服务商会自动分配。' +
				'<p>如果域名服务器IP填写错误，会造成网页无法打开，建议保持默认设置。'
		}, {
			id: 'm-1-8',
			text: "DHCP服务器",
			url: "lan_dhcps.asp",
			help: '<p>DHCP服务器提供了为客户端自动分配IP地址的功能，如果您使用本路由器的DHCP服务器功能的话，您可以让DHCP服务器自动替您配置局域网中各计算机的TCP/IP协议。' +
				'<p><em>IP地址池：</em>输入一个起始IP地址和一个终止IP地址以形成分配动态IP地址的范围。' +
				'<p><em>过期时间：</em>电脑(DHCP客户端)要求时才分配过期时间。'
		}, {
			id: 'm-1-9',
			text: "DHCP客户端列表",
			url: "lan_dhcp_clients.asp",
			help: '<p>DHCP客户端列表可以显示客户机从路由器DHCP服务器获取的IP地址，MAC地址，主机等信息。您可以手动输入IP和MAC地址，将它转换为静态分配。' +
				'<p><em>操作说明：</em>进行添加或删除后需点击确定按钮才能使操作生效。' +
				'<p><em>注意：</em>设置完成后，需重启路由器。'
		}
	]
};

MOD["wireless"] = {
	id: 'm-2',
	text: "无线设置",
	menus: [{
			id: 'm-2-1',
			text: "无线基本设置",
			url: "wireless_basic.asp",
			help: '<p>此页面只要对无线基本信息进行设置，建议只设置SSID和信道，其他选项保持默认。' +
				'<p><em>SSID：</em>无线网络中所有设备共享的网络名称。' +
				'<p><em>SSID广播：</em>当无线客户端在本地区域调查要关联的无线网络时，它们将通过路由器检测SSID广播。 如果选中，路由器将向所有的无线主机广播自己的SSID。' +
				'<p><em>信道：</em>您可以选择下拉列表中任何一个信道或者是自动模式，尽可能选择当前区域使用比较少的信道以避免干扰。' +
				'<p><em>扩展信道：</em>用于确定11n模式时本网络工作的频率段。'
		}, {
			id: 'm-2-2',
			text: "无线加密",
			url: "wireless_security.asp",
			help: '<p><em>WEP密码：</em>可以选择ASCII码（5或13个ASCII，禁止输入 \\\'"%）或者16进制数有效字符（10或26个16进制数）。' +
				'<p><em>WPA/WPA2：</em> 您可以启用个人或混合模式，但必须确定您的无线客户端支持该加密方式。' +
				'<p>在您对安全模式不是很熟悉的情况下，建议您使用"WPA-PSK模式"。您已经了解这几种安全模式，只要在您的客户端支持的情况下都可以选择。'
		}, {
			id: 'm-2-3',
			text: "无线访问控制",
			url: "wireless_filter.asp",
			help: '<p>使用无线访问控制功能可以根据PC的无线网卡MAC地址控制其是否可以与路由器进行通信。要禁用无线访问控制功能，请选关闭,要设置该功能请选择仅允许或仅禁止。'
		}, {
			id: 'm-2-4',
			text: "无线客户端",
			url: "wireless_state.asp",
			help: '<p>此页面可以查看到无线连接的客户端信息。' +
				'<p><em>MAC地址：</em>当前主机的无线网卡MAC地址。' +
				'<p><em>带宽：</em>信道频率宽度。'
		}
	]
};

MOD["bandwidth"] = {
	url: "net_tc.asp",
	text: "带宽控制",
	menus: [{
			id: 'm-3-1',
			text: "带宽控制",
			url: "net_tc.asp",
			help: '<p>这个功能可以限制内网计算机上网的通信流量。' +
				'<p><em>限速方向：</em>限制WAN口的上传带宽和下载带宽。' +
				'<p><em>带宽范围：</em>指定IP范围内的主机上传/下载最小到最大数据流量，单位KB/s 。上传/下载的上限不得超过WAN口的上限范围。' +
				'<p><em>1兆：</em>1Mbps=128KB/s ' +
				'<p><em>10兆：</em>10Mbps=1280KB/s'
		}, {
			id: 'm-3-2',
			text: "流量统计",
			url: "sys_iptAccount.asp",
			help: '<p>流量统计功能可以查看当前局域网内已连接的每台电脑的流量信息。'
		}
	]
};

MOD["specific"] = {
	id: 'm-4',
	text: "特殊应用",
	menus: [{
			id: 'm-4-1',
			text: "端口段映射",
			url: "nat_virtualportseg.asp",
			help: '<p><em>开始端口-结束端口：</em>广域网服务端口。' +
				'<p><em>启用：</em>选中时该项设置才生效。' +
				'<p><em>删除：</em>帮助您把该项的设置清空，然后点击确定生效。' +
				'<p><em>填充到：</em>帮助您把常用的服务端口写到您要设置的那一项的开始端口处。'
		}, {
			id: 'm-4-2',
			text: "DMZ主机",
			url: "nat_dmz.asp",
			help: '<p>在某些特殊情况下，需要让局域网中的一台计算机完全暴露给广域网，以实现双向通信，此时可以把该计算机设置为DMZ主机。键入一个DMZ主机的IP地址。启用DMZ之后，实际上就关闭了本设备对DMZ主机的防火墙保护。'
		}, {
			id: 'm-4-3',
			text: "DDNS",
			url: "ddns_config.asp",
			help: '<p>DDNS(动态 DNS)服务允许您为动态 WAN IP 地址分配一个固定域名，从而可监控您的 LAN 中的 Web, FTP 或其它 TCP/IP 。设定 DDNS 前，您只需点击"注册去"就可访问相应的网站，去注册一个域名。' +
				'<p><em>DDNS 服务：</em>从下拉菜单选择相应的DDNS服务提供商，并且输入用户名、密码 与域名 即可开启该服务。'
		}, {
			id: 'm-4-4',
			text: "UPNP设置",
			url: "upnp_config.asp",
			help: '<p>UPnP (通用即插即用)允许自动发现和设置联机在网络上的设备。 UPnP 目前仅被Windows ME和Windows XP及其以后的系统支持。'
		}, {
			id: 'm-4-5',
			text: "静态路由",
			url: "routing_static.asp",
			help: '<p><em>静态路由：</em>在网络中安装多个路由器时，您需要设置静态路由。静态路由功能确定数据沿网络前后通过路由器的路径。您可使用静态路由允许不同的 IP 域名用户通过此设备访问互联网。 请谨慎设置。' +
				'<p>在许多条件下最好使用动态路由，因为此功能允许路由器自动检测网络布局的物理变化。如果要使用静态路由，路由器的 DHCP 设定必须关闭。设置需要重新启动有才能生效。'
		}, {
			id: 'm-4-6',
			text: "路由列表",
			url: "routing_table.asp",
			help: '<p><em>跳跃数：</em>接口跃点数。' +
				'<p><em>接口：</em>有三种类型，vlan2:WAN口接口，ppp0:PPPoE接口，br0:内网设备接口。'
		}
	]
};

MOD["behave"] = {
	id: "m-5",
	text: "行为管理",
	menus: [{
			id: "m-5-1",
			text: "网站过滤",
			url: "firewall_urlfilter.asp",
			help: '<p>为了方便您对局域网中的计算机所能访问的网站进行控制，通过输入网站的URL可以阻止对某些网站的访问。详细使用请参考产品说明书。' +
				'<p><em>操作说明：</em>如果要清空已设置过的项，选中该项后点击清空按钮然后保存方生效。' +
				'<p><em>注意：</em>URL字符串：每个条目只能对应一个字符串。' + 
				'<p>时间设置为0:0~0:0表示全部时间段。'
		}, {
			id: "m-5-2",
			text: "MAC地址过滤",
			url: "firewall_mac.asp",
			help: '<p>您可以通过MAC地址过滤功能控制局域网中计算机对Internet的访问。详细使用请参考产品说明书。' +
				'<p><em>操作说明：</em>如果要清空已设置过的项，选中该项后点击清空按钮然后保存方生效。' +
				'<p><em>仅禁止：</em>仅禁止许条目内的IP或者IP段客户端的数据通过相应端口。' +
				'<p><em>仅允许：</em>仅允许条目内的IP或者IP段客户端的数据通过相应端口。' +
				'<p><em>注意：</em>时间设置为0:0~0:0表示全部时间段。'
		}, {
			id: "m-5-3",
			text: "端口过滤",
			url: "firewall_clientfilter.asp",
			help: '<p>为了方便您对局域网中的计算机进行进一步管理，您可以通过数据包过滤功能来控制局域网中计算机对相应端口的访问。 详细使用请参考产品说明书。' +
				'<p><em>操作说明：</em>如果要清空已设置过的项，选中该项后点击清空按钮然后保存方生效。' +
				'<p><em>仅禁止：</em>仅禁止条目内的IP或者IP段访问网络。' +
				'<p><em>仅允许：</em>仅允许条目内的IP或者IP段访问网络。' +
				'<p><em>注意：</em>时间设置为0:0~0:0表示全部时间段。'
		}
	]
};

MOD["system"] = {
	url: "system_reboot.asp",
	text: "系统工具",
	menus: [{
			id: 'm-6-1',
			text: "重启路由器",
			url: "system_reboot.asp",
			help: '<p>重启路由将使所改变的设置生效。在重启时，会自动断开所有连接，点击"确定"即可重启。'
		}, {
			id: 'm-6-2',
			text: "恢复出厂设置",
			url: "system_restore.asp",
			help: '<p>恢复出厂设置将使路由器的所有设置恢复到出厂时的默认状态。其中：' +
				'<p><em>默认的IP地址：</em>192.168.0.1' +
				'<p><em>默认的子网掩码：</em>255.255.255.0'
		}, {
			id: 'm-6-3',
			text: "备份/恢复设置",
			url: "system_backup.asp",
			help: '<p>单击"备份"便可以将当前配置以文件的形式备份到相应的目录，生成一个系统配置的备份文件。同理，您只需要点击"浏览"，选取相应目录中的配置文件，点击"恢复"，完成后重新启动路由将可以恢复到所需要的系统配置。'
		}, {
			id: 'm-6-4',
			text: "系统日志",
			url: "system_log.asp",
			help: '<p>查看历史操作记录' +
				'<p><em>提示：</em>如果日志满十五页(即150条记录)将会自动清空。'
		}, {
			id: 'm-6-5',
			text: "远程WEB管理",
			url: "system_remote.asp",
			help: '<p>使用此功能可以通过Internet从远程位置管理路由器。要启用此功能，请选择启用，然后使用 PC 上的指定端口来远程管理路由器。' +
				'<p><em>端口：</em>可以执行远程管理的端口号。' +
				'<p><em>IP地址：</em>广域网中可以执行远端WEB管理的计算机的IP地址。'
		}, {
			id: 'm-6-6',
			text: "网络时间",
			url: "system_hostname.asp",
			help: '<p>本页设置路由器的系统时间，您可以选择自己设置时间或者从互联网上获取标准的GMT时间，然后系统会自动连接NTP服务器进行时间同步。' +
				'<p><em>注意：</em>关闭路由器电源后，时间信息会丢失，当您下次开机连上Internet后，路由器将会自动获取GMT时间。' +
				'<p>您必须先连上Internet获取GMT时间或到此页设置时间后，其他功能（如防火墙）中的时间限定才能生效。'
		}, {
			id: 'm-6-7',
			text: "登录密码",
			url: "system_password.asp",
			help: '<p>路由器默认登录密码为空 建议更改路由器密码。如果不更改密码，您网络上的所有用户可访问路由器。' +
				'<p><em>旧密码：</em>输入旧密码。首次使用路由器时，不需要输入。( 注意：密码遗失或忘记就不能恢复。如果密码遗失或忘记，您必须让路由器复位到出厂默认设置)。' +
				'<p><em>新密码：</em>输入新密码。密码必须为 0 ～ 12 个字符，而且不含空格。' +
				'<p><em>确认新密码：</em>重新输入新密码进行确认。'
		}, {
			id: 'm-6-8',
			text: "软件升级",
			url: "system_upgrade.asp",
			help: '<p>登录我们公司的网站（www.tenda.com.cn），下载更高版本的软件。' +
				'<p>通过浏览找到相应的软件升级包。' +
				'<p>点击"升级"升级完成后，路由器将自动启动。'
		}
	]
};
		