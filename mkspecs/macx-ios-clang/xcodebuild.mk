
# We don't want xcodebuild to run in parallel
.NOTPARALLEL:

# Functions
targets = $(foreach target, $(EXPORT_SUBTARGETS), $(target)-$(strip $(1)))
toupper = $(shell echo $1 | tr '[:lower:]' '[:upper:]')
tolower = $(shell echo $1 | tr '[:upper:]' '[:lower:]')
basesdk = $(shell echo $1 | sed 's/[0-9.]*$$//')

# Explicit comma variable
, := ,

# Default targets
first: build
all: build_all

.DEFAULT_GOAL = first

# Top level targets
build: build_first
clean: clean_first
install: install_first
check: check_first
distclean: clean_all

$(EXPORT_SUBTARGETS): % : %-build

# Generic targets
%_first: $(firstword $(call targets, %)) ;
%_all: $(call targets, %) ;

# Actions
%-build: ACTION = build
%-build: xcodebuild-% ;

%-clean: ACTION = clean
%-clean: xcodebuild-% ;

%-install: ACTION = install
%-install: xcodebuild-% ;

# iOS Simulator doesn't support archiving
%-iphonesimulator-install: ACTION = build
iphonesimulator-install: ACTION = build

# Limit check to a single configuration
%-iphoneos-check: check-iphoneos ;
%-iphonesimulator-check: check-iphonesimulator ;

# SDK
%-iphoneos: SDK = iphoneos
%-iphonesimulator: SDK = iphonesimulator

# Configuration
release-%: CONFIGURATION = Release
debug-%: CONFIGURATION = Debug

# Test and build (device) destinations
ifneq ($(filter check%,$(MAKECMDGOALS)),)
  ifeq ($(DEVICES),)
    $(info Enumerating test destinations (you may override this by setting DEVICES explicitly), please wait...)
    SPECDIR := $(dir $(lastword $(MAKEFILE_LIST)))
    DESTINATIONS_INCLUDE = /tmp/ios_destinations.mk
    $(shell $(SPECDIR)/ios_destinations.sh $(TARGET) > $(DESTINATIONS_INCLUDE))
    include $(DESTINATIONS_INCLUDE)
  endif
endif

%-iphonesimulator: DEVICES = $(firstword $(IPHONESIMULATOR_DEVICES))
%-iphoneos: DEVICES = $(IPHONEOS_DEVICES)

IPHONEOS_GENERIC_DESTINATION := "generic/platform=iOS"
IPHONESIMULATOR_GENERIC_DESTINATION := "id=$(shell xcrun simctl list devices | grep -E 'iPhone|iPad' | grep -v unavailable | perl -lne 'print $$1 if /\((.*?)\)/' | tail -n 1)"

DESTINATION = $(if $(DESTINATION_ID),"id=$(DESTINATION_ID)",$(value $(call toupper,$(call basesdk,$(SDK)))_GENERIC_DESTINATION))

# Xcodebuild

DESTINATION_MESSAGE = "Running $(call tolower,$(CONFIGURATION)) $(ACTION) \
  on '$(DESTINATION_NAME)' ($(DESTINATION_ID))$(if $(DESTINATION_OS),$(,) $(DESTINATION_PLATFORM) $(DESTINATION_OS),)"

xcodebuild-%:
		@$(if $(DESTINATION_NAME), echo $(DESTINATION_MESSAGE),)
		xcodebuild $(ACTION) -scheme $(TARGET) $(if $(SDK), -sdk $(SDK),) $(if $(CONFIGURATION), -configuration $(CONFIGURATION),) $(if $(DESTINATION), -destination $(DESTINATION) -destination-timeout 1,)

xcodebuild-check-device_%: DESTINATION_ID=$(lastword $(subst _, ,$@))

# Special check target (requires SECONDEXPANSION due to devices)
.SECONDEXPANSION:
check-%: ACTION = test
check-%: $$(foreach device, $$(DEVICES), xcodebuild-check-device_$$(device)) ;
	  @echo $(if $^, Ran $(call tolower,$(CONFIGURATION)) tests on $(words $^) $(SDK) destination\(s\): $(DEVICES), No compatible test devices found for \'$(SDK)\' SDK && false)

# Determined by device
check-%: SDK =

# Default to debug for testing
check-%: CONFIGURATION = Debug

