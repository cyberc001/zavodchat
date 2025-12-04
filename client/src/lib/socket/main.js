export default class MainSocket {
	static host;

	ws = new WebSocket(MainSocket.host);
};
