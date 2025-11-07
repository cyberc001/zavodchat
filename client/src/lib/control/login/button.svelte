<script>
	const {text, onClick} = $props();
</script>

<button class="login_button" on:click={onClick}>{text}</button>

<style>
.login_button {
	font-size: 20px;
	color: #F6F5F5;

	background: #2A2E34;

	border-color: #767676;
	border-width: 2px;
	border-style: solid;
	border-radius: 4px;
}
.login_button:focus {
	outline: none;
	border-color: #C8C0C0;
}
.login_button:active {
	border-style: outset;
}
.login_button::selection {
	background: #644646;
}
</style>
