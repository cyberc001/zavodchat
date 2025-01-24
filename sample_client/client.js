// токены и прочие данные хранятся в локальных переменных
var auth_token

// запросы

function login(username, password) {
	$.get(`https://localhost/auth?username=${username}&password=${password}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })
		.done(function(data) {
			auth_token = data
			$("#label_token").html(auth_token)
			$("#label_login").html(username)

			get_servers()
		})
}

function get_servers() {
	$.get(`https://localhost/servers?token=${auth_token}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })	
		.done(function(data) {
			data = JSON.parse(data)

			data_servers = {}
			let server_html = []
			for(const serv of data){
				server_html.push(`<input type="button" value="${serv.name}" id="serv_${serv.id}" onclick="get_channels(${serv.id})" style="margin-top: 3px"/><br>`)
			}
			$("#server_panel").html(server_html.join(""))
		})
}

function get_channels(server_id) {
	$.get(`https://localhost/servers/${server_id}/channels?token=${auth_token}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })	
		.done(function(data) {
			data = JSON.parse(data)
			$("#label_server").html(server_id)

			let channel_html = []
			for(const chan of data){
				if(chan.type == 0)
					channel_html.push(`<input type="button" value="${chan.name}" id="chan_${chan.id}" onclick="get_messages(${server_id}, ${chan.id})" style="margin-top: 3px"/><br>`)
				else if(chan.type == 1)
					channel_html.push(`<input type="button" value="${chan.name}" id="chan_${chan.id}" onclick="join_vc(${chan.id})" style="margin-top: 3px"/><br>`)
			}
			$("#channel_panel").html(channel_html.join(""))
		})
}

function get_messages(server_id, channel_id) {
	$.get(`https://localhost/servers/${server_id}/channels/${channel_id}/messages?token=${auth_token}&start=0&count=50`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })
		.done(function(data) {
			data = JSON.parse(data)
			$("#label_channel").html(channel_id)

			let message_html = []
			for(const msg of data){
				message_html.push(`<b>#${msg.author_id}: </b><a>${msg.text}</a><br>`)
			}
			$("#message_panel").html(message_html.join(""))
		})
}


// сокеты

var vc_sock

function join_vc(channel_id) {
	if(vc_sock)
		vc_sock.close(1000, "by_user");

	vc_sock = new WebSocket(`wss://localhost:445?token=${auth_token}&channel=${channel_id}`)
	vc_sock.onopen = function(ev) {
		alert("connected to vc")
	}
	vc_sock.onclose = function(ev) {
		if(ev.reason != "by_user")
			alert("vc socket closed: " + ev.reason)
	}
	vc_sock.onmessage = async function(ev) {
		const offer = JSON.parse(ev.data)
		const rtc_conn = new RTCPeerConnection({
			bundlePolicy: 'max-bundle'
		});

		rtc_conn.ontrack = (ev) => {
			console.log("track event", ev);
		};

		console.log("setting to offer ", offer);
		await rtc_conn.setRemoteDescription(offer);
		const answer = rtc_conn.createAnswer().then(function(result){
			console.log(result);
			vc_sock.send(JSON.stringify(result));
		});
		await rtc_conn.setLocalDescription(answer);
	}
}
