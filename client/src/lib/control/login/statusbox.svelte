<script>
	const {text, is_error = true} = $props();
	let color = $derived(is_error ? "#644646" : "#4c6446");
</script>

<p class="login_statusbox" style:background={color}>{text}</p>

<style>
.login_statusbox {
	font-size: 16px;
	max-width: inherit;
}
</style>
