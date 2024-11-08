Примечания:
* Все методы, которые требуют заголовок "token", могут вернуть ошибку 401, если пользователь использует неизвестный/истёкший токен. В таком случае надо попробовать авторизоваться заново, возможно через сохранённый логин/пароль. Если ошибка повторяется, выкинуть пользователя на экран авторизации.
* Все методы `/servers/$id/...` могут вернуть ошибку 403, если пользователь не состоит на сервере, или не имеет достаточно прав на изменение его настроек; также они могут вернуть ошибку 404, если сервера не существует. В документации эти ошибки не указываются.
* Все методы `/servers/$id/channels/$id/...` могут вернуть ошибку 403, если пользователь не имеет достаточно прав на изменение канала; также они могут вернуть ошибку 404, если канала не существует. В документации эти ошибки не указываются.

## /auth

**POST:**

Вход в систему по логину/паролю.

* username: string<br>
Логин
* password: string<br>
Пароль

*response 200:*

Авторизация прошла успешно.

body: `<временный токен>`

*response 404:*

Неверный логин / пароль.


**PUT:**

Регистрация в системе.

*headers:*
* username: string<br>
Логин
* displayname: string<br>
Отображаемое имя
* password: string<br>
Пароль

*response 200:*

Регистрация прошла успешно.

*response 403:*

Пользователь с таким именем уже существует.

## /users/$id

**GET:**

Получить информацию о пользователе.

*response 200:*

body: JSON-объект с информацией о пользователе.
```
{
	"id": 412,
	"name": "cyb3rc001",
	"avatar": "123.12.53.45/avatar_453912.png",
	"status": "online"
}
```
*response 404:*

Пользователя с таким $id не существует.

## /servers

**GET:**

Получить список серверов, в которых состоит пользователь.

*headers:*
* token: uint64_t

*response 200:*

body: JSON-массив из объектов - информации о каждом сервере.

см. `/servers/$id GET`

**PUT:**

Создать сервер

*headers:*
* token: uint64_t
* name: string<br>
Имя сервера
	
*response 200:*

body: `<ID созданного сервера>`

*response 403:*

Превышено ограничение на количество серверов, которыми владеет пользователь.

body: `<причина ошибки>`

## /servers/$id

**GET:**

Получить информацию о сервере

*headers:*
* token: uint64_t

*response 200:*

body: JSON-объект с информацией о севрере.
```
{
	"name": "zona bikini",
	"avatar": "123.12.53.45/avatar_453912.png",
	"channel_count": 4,
	"user_count": 10
}
```
**DELETE:**

Удалить сервер

*headers:*
* token: uint64_t
	
*response 200:*

Сервер успешно удалён.


## /servers/$id/users

**GET:**

Получить список пользователей в определённом диапазоне индексов.

*headers:*
* token: uint64_t
* start: int<br>
Индекс, с которого начинается получение пользователей.
* count: int<br>
Количество получаемых пользователей.

*response 200:*

body: JSON-массив из объектов - информации о каждом пользователей. Если count выходит за пределы массива пользователей, то метод возвращает столько пользователей, сколько возможно.

см. `/users/$id GET`

*response 403:*
* Неправильный (< 0 или >= количеству пользователей) индекс.
* count не поддерживается сервером (> 50).

body: `<причина ошибки>`
	
## /servers/$id/channels

**GET:**

Получить список каналов

*headers:*
* token: uint64_t

*response 200:*

body: JSON-массив из объектов - информации о каждом канале.

см. `/servers/$id/channels/$id GET`

**PUT:**

Создать канал

*headers:*
* token: uint64_t
* name: string<br>
Имя канала
* type: string<br>
Тип канала - text/voice

*response 200:*

body: `<ID созданного канала>`

*response 403:*

Превышено ограничение на количество каналов на сервере.

body: `<причина ошибки>`

## /servers/$id/channels/$id

**GET:**

Получить информацию о канале

*headers:*
* token: uint64_t

*response 200:*

body: JSON-объект с информацией о канале.

Пример:
```
{
	"id": 4,
	"type": "text",
	"name": "general"
}
```

**POST:**

Изменить параметры канала

*headers:*
* token: uint64_t
* name: string<br>
Имя канала
* type: string<br>
Тип канала - text/voice

*response 200:*

Параметры успешно изменены.

**DELETE:**

Удалить канал

*headers:*
* token: uint64_t

*response 200:*

Канал успешно удалён.

## /servers/$id/channels/$id/messages/count

**GET:**

Получить количество сообщений в канале.

*headers:*
* token: uint64_t

*response 200:*

body: `<Количество сообщений>`

*response 403:*

Канал не является текстовым.

body: `<причина ошибки>`

## /servers/$id/channels/$id/messages

**GET:**

Получить сообщения в определённом диапазоне индексов.

*headers:*
* token: uint64_t
* start: int<br>
Индекс, с которого начинается получение сообщений.
* count: int<br>
Количество получаемых сообщений.

*response 200:*

body: JSON-массив из объектов - информации о каждом сообщении. Если count выходит за пределы массива сообщений, то метод возвращает столько сообщений, сколько возможно.

см. `/servers/$id/channels/$id/messages/$id GET`

*response 403:*
* Неправильный (< 0 или >= количеству сообщений) индекс.
* count не поддерживается сервером (> 50).
* Канал не является текстовым.

body: `<причина ошибки>`

## /servers/$id/channels/$id/messages/$id

**GET:**

Получить информацию о сообщении с определённым ID.

*headers:*
* token: uint64_t

*response 200:*

body: JSON-объект с информацией о сообщении.

Пример:
```
{
		"id": 412,
		"text": "папа ты купил молоко?",
		"author_id": 341,
		"sent": "2024-12-17 07:37:16+10",
		"edited": "2024-12-18 09:43:05+10"
}
```
