GENERATOR = 3rdparty/x86simd_generate.pl
TARGETDIR = ../../src/corelib/global/
TARGETCPP = qsimd_x86.cpp
TARGETHEADER = qsimd_x86_p.h

CONF_FILES = 3rdparty/simd-intel.conf
# We don't currently use any feature from simd-amd.conf
# CONF_FILES += 3rdparty/simd-amd.conf

all: $(TARGETDIR)/$(TARGETHEADER) $(TARGETDIR)/$(TARGETCPP)
$(TARGETHEADER): $(CONF_FILES) | $(GENERATOR)
	cat $^ | perl $(GENERATOR) /dev/stdin $@ > $@
$(TARGETDIR)/$(TARGETHEADER): header $(TARGETHEADER)
	sed '/-- implementation start --/,/-- implementation end --/d' $^ | \
	sed 's!3rdparty/x86simd_generate\.pl!util/x86simdgen/README.md!' > $@
$(TARGETDIR)/$(TARGETCPP): $(TARGETHEADER) header
	(sed '/^$$/q' header; \
	 echo '#include "$(TARGETHEADER)"'; \
	 sed '1,/-- implementation start --/d;/-- implementation end --/,$$d' $<) > $@

clean:
	-$(RM) $(TARGETHEADER)
