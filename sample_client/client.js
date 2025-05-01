var hostname = "localhost"

// токены и прочие данные хранятся в локальных переменных
var auth_token

// запросы
function login(username, password) {
	$.get(`https://${hostname}/auth?username=${username}&password=${password}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })
		.done(function(data) {
			auth_token = data
			$("#label_token").html(auth_token)
			$("#label_login").html(username)

			get_servers()
		})
}

function get_servers() {
	$.get(`https://${hostname}/servers?token=${auth_token}`)
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
	$.get(`https://${hostname}/servers/${server_id}/channels?token=${auth_token}`)
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
	$.get(`https://${hostname}/servers/${server_id}/channels/${channel_id}/messages?token=${auth_token}&start=0&count=50`)
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


// голосовой чат
var vc_sock
var rtc_conn

var vc_audio_elem = document.getElementById('vc_audio')
var elem_to_track = {} // словарь, отображающий "id дорожки" -> "ссылка на DOM-элемент" (id элементов уже заняты user_id)

function join_vc(channel_id) {
	if(vc_sock)
		vc_sock.close(1000, "by_user");

	vc_sock = new WebSocket(`wss://${hostname}:445?token=${auth_token}&channel=${channel_id}`)
	vc_sock.onopen = function(ev) {
		alert("connected to vc")
	}
	vc_sock.onclose = function(ev) {
		if(ev.reason != "by_user")
			alert("vc socket closed: " + ev.reason)
	}
	vc_sock.onmessage = async function(ev) {
		const offer = JSON.parse(ev.data)

		if(!rtc_conn){
			rtc_conn = new RTCPeerConnection({
				bundlePolicy: 'max-bundle'
			})

			rtc_conn.ontrack = (ev) => {
				let user_id = Number(ev.transceiver.mid)
				let audio_elem_id = 'vc_audio_' + user_id

				if(!ev.streams[0].onremovetrack){
					ev.streams[0].onremovetrack = (ev) => {
						console.log("remove track event", ev)
						elem_to_track[ev.track.id].remove()
						delete elem_to_track[ev.track.id]
					}
				}

				if(!document.getElementById(audio_elem_id)){
					console.log("add track event", ev)
					let audio_elem = document.createElement('audio')
					audio_elem.id = audio_elem_id
					elem_to_track[ev.track.id] = audio_elem
					audio_elem.controls = 'controls'
					audio_elem.srcObject = new MediaStream()
					audio_elem.srcObject.addTrack(ev.track)

					vc_audio_elem.appendChild(audio_elem)
					audio_elem.play()
				}

				/*console.log("generic track event", ev)
				const vc_audio = $(`#vc_audio_${last_track_id}`)[0]

				let user_id = Number(ev.transceiver.mid)
				let track_id = last_track_id++
				track_ids[user_id] = track_id

				vc_audio.srcObject = new MediaStream()
				vc_audio.srcObject.addTrack(ev.track)
				vc_audio.play()
				ev.streams[0].onremovetrack = (ev) => {
					console.log("remove track event", ev)
				}*/
			}

			rtc_conn.onsignalingstatechange = (state) => {
				if(rtc_conn.signalingState === "stable"){
					const answer = rtc_conn.localDescription
					console.log("sending answer:")
					console.log(answer)
					vc_sock.send(JSON.stringify(answer))
				}
			}

			const media = await navigator.mediaDevices.getUserMedia({audio: true})
			media.getTracks().forEach(track => {rtc_conn.addTrack(track, media); console.log("added track", track)})
		}

		console.log("setting to offer ", offer)
		await rtc_conn.setRemoteDescription(offer)

		const answer = await rtc_conn.createAnswer()
		await rtc_conn.setLocalDescription(answer)
	}
}
