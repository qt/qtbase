TEMPLATE = subdirs

sd = $$files(*)
for(d, sd): \
    exists($$d/$${d}.pro): \
        SUBDIRS += $$d
