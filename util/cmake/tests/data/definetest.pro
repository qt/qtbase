defineTest(pathIsAbsolute) {
    p = $$clean_path($$1)
    !isEmpty(p):isEqual(p, $$absolute_path($$p)): return(true)
    return(false)
}

