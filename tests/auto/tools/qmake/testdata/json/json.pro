jsontext = $$cat($$PWD/test.json)
parseJson(jsontext, json)

# print all keys
message(json._KEYS_ $${json._KEYS_})

# print array
message(json.array._KEYS_ $${json.array._KEYS_})
for(key, json.array._KEYS_): \
    message(json.array.$${key} $$eval(json.array.$${key}))

# print object
message(json.object._KEYS_ $${json.object._KEYS_})
for(key, json.object._KEYS_): \
    message(json.object.$${key} $$eval(json.object.$${key}))

# print value tyes
message(json.string: $${json.string})
message(json.number: $${json.number})
message(json.true: $${json.true})
message(json.false: $${json.false})
message(json.null: $${json.null})

# check that booleans work
$${json.true}: message(json.true is true)
!$${json.false}: message(json.false is false)
