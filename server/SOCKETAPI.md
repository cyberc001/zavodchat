Все события сокетов имеют следующую форму:
```js
{
	data:
	{
		// объект с данными...
	},
	name: "название события"
}
```

## Сообщения

### message_created

В канале, к которому имеет доступ пользователь, было создано новое сообщение.

Пример:

```js
{
	"data":
	{
		"author_id": 1,
		"channel_id": 1,
		"edited": "2024-12-04 18:36:14.045681+00",
		"id": 6,
		"sent": "2024-12-04 18:36:14.045681+00",
		"server_id": 1,
		"text": "its over guys"
	},
	"name": "message_created"
}
```

### message_edited

В канале, к которому имеет доступ пользователь, было отредактировано существующее сообщение.

Пример:

```js
{
	"data":
	{
		"channel_id": 1,
		"edited": "2025-01-11 01:05:03.080518+00",
		"id": 6,
		"server_id": 1,
		"text": "you_are_good_people"
	},
	"name": "message_edited"
}
```

### message_deleted

В канале, к которому имеет доступ пользователь, было удалено существующее сообщение.

Пример:

```js
{
	"data":
	{
		"channel_id": 1,
		"id": 8,
		"server_id": 1
	},
	"name": "message_deleted"
}
```

## Каналы

### channel_created

На сервере, в котором состоит пользователь, был создан канал.

Пример:

```js
{
	"data":
	{
		"id": 7,
		"name": "political_discussion",
		"server_id": 1,
		"type": 0
	},
	"name": "channel_created"
}
```

### channel_edited

На сервере, в котором состоит пользователь, был изменён канал.

Пример:

```js
{
	"data":
	{
		"id": 7,
		"name": "shared_photos",
		"server_id": 1,
		"type": 1
	},
	"name": "channel_edited"
}
```

### channel_deleted

На сервере, в котором состоит пользователь, был удалён канал.

Пример:

```js
{
	"data":
	{
		"id": 7,
		"server_id": 1
	},
	"name": "channel_deleted"
}
```

## Серверы

### server_edited

Сервер, в котором состоит пользователь, был изменён.

```js
{
	"data": 
	{
		"id": 1,
		"name": "speed_cameras_hotspots"
	},
	"name": "server_edited"
}
```

### server_deleted

Сервер, в котором состоит пользователь, был удалён (вызывается сразу перед удалением сервера из базы данных).

Пример:

```js
{
	"data":
	{
		"id": 2
	},
	"name": "server_deleted"
}
```

## Пользователи на сервере

### user_joined

Пользователь присоединился к серверу.

Пример:

```js
{
	"data":
	{
		"id": 2,
		"server_id": 1
	},
	"name": "user_joined"
}
```

### user_left

Пользователь вышел с сервера (кик/бан). Причина удаления пользователя с сервера не разглашается.
Это событие приходит в том числе самому пользователю.

Пример:

```js
{
	"data":
	{
		"id": 1,
		"server_id": 1
	},
	"name": "user_left"
}
```

## Пользователь

### got_server_owner

Пользователю передали права владельца сервера.

Пример:

```js
{
	"data":
	{
		"id": 1
	},
	"name": "got_server_owner"
}
```