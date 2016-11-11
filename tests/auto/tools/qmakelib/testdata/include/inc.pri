VAR = val
.VAR = nope

PLUS += more

fake-*: MATCH = 1

defineTest(func) {
    message("say hi!")
}

defineReplace(func) {
    return("say hi!")
}
