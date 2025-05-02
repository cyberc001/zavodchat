var hostname = 'localhost'

// токены и прочие данные хранятся в локальных переменных
var auth_token

// запросы
function login(username, password)
{
	$.get(`https://${hostname}/auth?username=${username}&password=${password}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })
		.done(function(data) {
			auth_token = data
			$('#label_token').html(auth_token)
			$('#label_login').html(username)

			get_servers()
		})
}

function get_servers()
{
	$.get(`https://${hostname}/servers?token=${auth_token}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })	
		.done(function(data) {
			data = JSON.parse(data)

			data_servers = {}
			let server_html = []
			for(const serv of data){
				server_html.push(`<input type='button' value='${serv.name}' id='serv_${serv.id}' onclick='get_channels(${serv.id})' style='margin-top: 3px'/><br>`)
			}
			$('#server_panel').html(server_html.join(''))
		})
}

function get_channels(server_id)
{
	$.get(`https://${hostname}/servers/${server_id}/channels?token=${auth_token}`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })	
		.done(function(data) {
			data = JSON.parse(data)
			$('#label_server').html(server_id)

			let channel_html = []
			for(const chan of data){
				if(chan.type == 0)
					channel_html.push(`<input type='button' value='${chan.name}' id='chan_${chan.id}' onclick='get_messages(${server_id}, ${chan.id})' style='margin-top: 3px'/><br>`)
				else if(chan.type == 1)
					channel_html.push(`<input type='button' value='${chan.name}' id='chan_${chan.id}' onclick='join_vc(${chan.id})' style='margin-top: 3px'/><br>`)
			}
			$('#channel_panel').html(channel_html.join(''))
		})
}

function get_messages(server_id, channel_id)
{
	$.get(`https://${hostname}/servers/${server_id}/channels/${channel_id}/messages?token=${auth_token}&start=0&count=50`)
		.fail(function(jqxhr, _status, _error) { alert(jqxhr.responseText) })
		.done(function(data) {
			data = JSON.parse(data)
			$('#label_channel').html(channel_id)

			let message_html = []
			for(const msg of data){
				message_html.push(`<b>#${msg.author_id}: </b><a>${msg.text}</a><br>`)
			}
			$('#message_panel').html(message_html.join(''))
		})
}


// голосовой чат
var vc_sock
var rtc_conn

var vc_audio_elem = document.getElementById('vc_audio')
var track_to_user_id = [] // индекс = (ID трека - 1); значение = ID пользователя (null если трек не используется)

function parse_sdp_user_ids(sdp)
{
	// заново заполнить track_to_user_id
	for(let i = 0; i < track_to_user_id.length; ++i)
		track_to_user_id[i] = null

	// присутствуют атрибуты mid и user_id -> дорожка используется
	let lines = sdp.split('\n')
	let mid = null
	let user_id = null
	for(let i = 0; i < lines.length; ++i){
		let l = lines[i]
		if(l[0] == 'm' || i == lines.length - 1){
			if(mid !== null && user_id !== null){
				track_to_user_id[Number(mid) - 1] = Number(user_id)
				mid = null
				user_id = null
			}
		} else if(l.startsWith('a=user:'))
			user_id = l.substring(7)
		else if(l.startsWith('a=mid:'))
			mid = l.substring(6)
	}

	// убрать неиспользуемые треки (и показать вновь используемые) со страницы (чисто визуально)
	for(let i = 0; i < track_to_user_id.length; ++i)
		document.getElementById('vc_audio_' + (i + 1)).style.display = track_to_user_id[i] ? 'block' : 'none';
}

function join_vc(channel_id)
{
	if(vc_sock)
		vc_sock.close(1000, 'by_user');

	vc_sock = new WebSocket(`wss://${hostname}:445?token=${auth_token}&channel=${channel_id}`)
	vc_sock.onopen = function(ev) {
		alert('connected to vc')
	}
	vc_sock.onclose = function(ev) {
		if(ev.reason != 'by_user')
			alert('vc socket closed: ' + ev.reason)
	}
	vc_sock.onmessage = async function(ev) {
		const offer = JSON.parse(ev.data)

		if(!rtc_conn){
			rtc_conn = new RTCPeerConnection({
				bundlePolicy: 'max-bundle'
			})

			rtc_conn.ontrack = (ev) => {
				let audio_elem_id = 'vc_audio_' + ev.transceiver.mid
				// создать новый элемент воспроизведения аудио, если появился новый трек
				if(!document.getElementById(audio_elem_id)){
					track_to_user_id.push(null)

					console.log('add track event', ev)
					let audio_elem = document.createElement('audio')
					audio_elem.id = audio_elem_id
					audio_elem.controls = 'controls'
					audio_elem.srcObject = new MediaStream()
					audio_elem.srcObject.addTrack(ev.track)

					vc_audio_elem.appendChild(audio_elem)
					audio_elem.play()
				}
			}

			rtc_conn.onsignalingstatechange = (state) => {
				if(rtc_conn.signalingState === 'stable'){
					const answer = rtc_conn.localDescription
					console.log('sending answer:')
					console.log(answer)
					vc_sock.send(JSON.stringify(answer))
				}
			}

			const media = await navigator.mediaDevices.getUserMedia({audio: true})
			media.getTracks().forEach(track => {rtc_conn.addTrack(track, media); console.log('added track', track)})
		}

		console.log('setting to offer ', offer)
		await rtc_conn.setRemoteDescription(offer)
		parse_sdp_user_ids(offer.sdp)

		const answer = await rtc_conn.createAnswer()
		await rtc_conn.setLocalDescription(answer)
	}
}
