$.get("https://localhost/auth?username=test&password=qwe123")
	.fail(function(jqxhr, _status, _error) { console.log(_error); })
	.done(function(data) { console.log(data); })
