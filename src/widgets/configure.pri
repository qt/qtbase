# custom outputs

defineTest(qtConfOutput_styles) {
    !$${2}: return()

    style = $$replace($${1}.feature, "style-", "")
    qtConfOutputVar(append, "privatePro", "styles", $$style)
}
