TEMPLATE      = subdirs
SUBDIRS       = basicdrawing \
                concentriccircles \
                examples_affine \
                examples_composition \
                examples_deform \
                examples_gradients \
                examples_pathstroke \
                painting_shared \
                imagecomposition \
                painterpaths \
                transformations \
                fontsampler

examples_affine.subdir = affine
examples_composition.subdir = composition
examples_deform.subdir = deform
examples_gradients.subdir = gradients
examples_pathstroke.subdir = pathstroke
painting_shared.subdir = shared

!ordered {
    examples_affine.depends = painting_shared
    examples_deform.depends = painting_shared
    examples_gradients.depends = painting_shared
    examples_composition.depends = painting_shared
    examples_pathstroke.depends = painting_shared
}
