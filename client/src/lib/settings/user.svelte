<script module>
	export function tabs() {
		return [
			{ name: "profile", render: profile },
			{ name: "test2" }
		];
	}
</script>

{#snippet profile()}
123
{/snippet}
