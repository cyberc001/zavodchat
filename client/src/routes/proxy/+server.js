export async function GET({request}){
	const ctrl = new AbortController();
	setTimeout(() => ctrl.abort(), 5000);
	try {
		return new Response(await (await fetch(request.headers.get("link"), {signal: ctrl.signal})).text());
	} catch {
		return new Response(null, {status: 400});
	}
}