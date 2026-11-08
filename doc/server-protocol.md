# Server protocol

This is a new protocol contract: the tobuy-server must implement it for the
device to work. All communication is unauthenticated and unencrypted; use it
only on a trusted network. Anyone on that network can read or change the list
and impersonate the server. Configure an `http://host[:port]` base URL; port
defaults to 80. The device connects to `ws://host[:port]/ws`, then fetches
`GET http://host[:port]/tobuy`. On every WebSocket reconnect it fetches again.
The server must answer WebSocket ping frames with pong frames. For device
configuration, see [device setup](device-setup.md).

## List response and updates

The HTTP response must be status 200 and a UTF-8 JSON object. The server sends
the **same complete object** as a single WebSocket text message whenever the
list changes, including in response to accepted switches:

```json
{
  "type": "list",
  "revision": 42,
  "periodic": [
    { "id": "milk", "name": "Milk", "needed": true },
    { "id": "coffee", "name": "Coffee", "needed": false }
  ],
  "missing": [
    { "id": "bread", "name": "Bread" }
  ]
}
```

`revision` is an unsigned 32-bit integer, increasing for every changed list;
equal revisions must describe identical lists. The server must keep revisions
monotonic across the HTTP response and WebSocket updates for a connection.
The device ignores older revisions to handle a delayed HTTP response. After a
reconnect it accepts a fresh revision sequence, including after server restart.
IDs are nonempty unique strings across both arrays, at most 32 UTF-8 bytes;
names are nonempty strings of at most 64 UTF-8 bytes. `needed` is a JSON
boolean and required for periodic items. Both arrays are required, may be
empty, and may contain at most 64 items combined. Messages must be no larger
than 8192 UTF-8 bytes and must not be fragmented WebSocket messages. Names
are rendered with the TFT font and wrap over multiple lines; unsupported characters
may not display correctly. Invalid or oversized lists are rejected, preserving the
last valid list and displaying an error.

## Periodic item changes

When a user taps a periodic switch the device sends one WebSocket text message:

```json
{ "type": "set_periodic", "id": "milk", "needed": true }
```

The server should apply this idempotent desired state and broadcast a complete,
updated `list` object with a newer revision. It should also broadcast the
current list when the item is already in that state. The device permits one
outstanding request and does not change the switch until a list confirms the
desired state. If confirmation does not arrive within 10 seconds, it reports
an error and refetches; rejected changes therefore remain unchanged. Other
server message types are not supported. Connection failures are shown on the
display and retried; HTTP requests have bounded network timeouts but can
briefly pause touch handling.

## Temperature

The temperature screen requests `GET /temperature` from the same HTTP server.
The response must be a JSON object with a numeric `temperature`, a `unit` of
`°C`, a nonempty `city`, and an optional or null `country`, as specified by
the [server documentation](https://github.com/wil92/tobuy-server/blob/master/doc/temperature.md).
Place names are limited to 96 UTF-8 bytes and the complete response to 1024
bytes. The device rejects malformed, oversized, or non-Celsius responses.
Long names are clipped on screen, and unsupported font characters may not render.

While the temperature screen is open, a reading is requested immediately if
needed and refreshed every 10 minutes. Failed requests retry after 30 seconds;
Wi-Fi reconnection triggers a retry immediately. The last valid reading remains
visible with an error, or a loading message during retries, until a new reading
succeeds. The weather request runs
in a separate task so the server's upstream lookups do not block touch or
shopping updates. The server requires `WEATHER_CITY` to be configured and
can return 404, 502, or 503 for weather errors.
