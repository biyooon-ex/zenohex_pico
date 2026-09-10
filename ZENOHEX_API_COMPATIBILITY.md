# Zenohex API compatibility

This table maps [Zenohex](https://github.com/biyooon-ex/zenohex) APIs to their ZenohexPico counterparts. `---` indicates that no corresponding ZenohexPico API is available.

| Zenohex | ZenohexPico |
| --- | --- |
| Zenohex.put/3 | ZenohexPico.put/4 |
| Zenohex.delete/2 | --- |
| Zenohex.get/3 | ZenohexPico.get/4 |
| Zenohex.scout/3 | --- |
| Zenohex.Config.default/0 | ZenohexPico.Config.default/0 |
| Zenohex.Config.from_env/0 | --- |
| Zenohex.Config.from_file/1 | --- |
| Zenohex.Config.from_json5/1 | --- |
| Zenohex.Config.get_json/2 | ZenohexPico.Config.get/2 |
| Zenohex.Config.insert_json5/3 | ZenohexPico.Config.insert/3 |
| Zenohex.Config.insert_json5_array_item/3 | --- |
| Zenohex.Config.remove_json5_array_item/2 | --- |
| Zenohex.ConfigMap.default/0 | --- |
| Zenohex.ConfigMap.from_env/0 | --- |
| Zenohex.ConfigMap.from_file/1 | --- |
| Zenohex.ConfigMap.from_json5/1 | --- |
| Zenohex.ConfigMap.get/2 | --- |
| Zenohex.ConfigMap.insert/3 | --- |
| Zenohex.ConfigMap.merge/2 | --- |
| Zenohex.Session.open/0 | --- |
| Zenohex.Session.open/1 | ZenohexPico.Session.open/1 |
| Zenohex.Session.close/1 | ZenohexPico.Session.close/1 |
| Zenohex.Session.closed?/1 | --- |
| Zenohex.Session.put/4 | ZenohexPico.Session.put/4 |
| Zenohex.Session.delete/3 | --- |
| Zenohex.Session.get/4 | ZenohexPico.Session.get/4 |
| Zenohex.Session.new_timestamp/1 | --- |
| Zenohex.Session.info/1 | --- |
| Zenohex.Session.declare_publisher/3 | --- |
| Zenohex.Session.declare_subscriber/4 | ZenohexPico.Session.declare_subscriber/4 |
| Zenohex.Session.declare_querier/3 | --- |
| Zenohex.Session.declare_queryable/4 | --- |
| Zenohex.Publisher.put/3 | --- |
| Zenohex.Publisher.delete/2 | --- |
| Zenohex.Publisher.undeclare/1 | --- |
| Zenohex.Subscriber.undeclare/1 | ZenohexPico.Subscriber.undeclare/1 |
| Zenohex.Querier.get/3 | --- |
| Zenohex.Querier.get_async/3 | --- |
| Zenohex.Querier.undeclare/1 | --- |
| Zenohex.Queryable.undeclare/1 | --- |
| Zenohex.Query.reply/4 | --- |
| Zenohex.Query.reply_error/3 | --- |
| Zenohex.Query.reply_delete/3 | --- |
| Zenohex.Liveliness.get/4 | --- |
| Zenohex.Liveliness.declare_subscriber/3 | --- |
| Zenohex.Liveliness.undeclare_subscriber/1 | --- |
| Zenohex.Liveliness.declare_token/2 | --- |
| Zenohex.Liveliness.undeclare_token/1 | --- |
| Zenohex.Matching.status/1 | --- |
| Zenohex.Matching.declare_listener/2 | --- |
| Zenohex.Matching.undeclare_listener/1 | --- |
| Zenohex.Scouting.scout/3 | --- |
| Zenohex.Scouting.declare_scout/3 | --- |
| Zenohex.Scouting.stop_scout/1 | --- |
| Zenohex.KeyExpr.canonize/1 | --- |
| Zenohex.KeyExpr.join/2 | --- |
| Zenohex.KeyExpr.valid?/1 | --- |
| Zenohex.KeyExpr.intersects?/2 | --- |
| Zenohex.KeyExpr.includes?/2 | --- |

## Notable API differences

- `Zenohex.Config.default/0` returns a JSON binary directly, whereas `ZenohexPico.Config.default/0` returns `{:ok, reference()}` on success (or `{:error, term()}`).
- `ZenohexPico.Config.get/2` and `ZenohexPico.Config.insert/3` accept only supported atom keys. See the `ZenohexPico.Config` documentation for the list of supported keys. In contrast, `Zenohex.Config.get_json/2` and `Zenohex.Config.insert_json5/3` use string key paths and JSON5 values.
- `Zenohex.put/3` and `Zenohex.get/3` use the default configuration internally. Their `ZenohexPico` counterparts require a `ZenohexPico.Config.t()` as the first argument.
