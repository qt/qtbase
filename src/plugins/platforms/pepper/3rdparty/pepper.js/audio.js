// Copyright (c) 2013 Google Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(function() {

  var isAudioSupported = function() {
    return !!(window["AudioContext"] || window["webkitAudioContext"]);
  };

  var createAudioContext = function() {
    if (window["AudioContext"] !== undefined) {
      return new AudioContext();
    } else {
      return new webkitAudioContext();
    }
  }

  // The Web Audio API currently does not allow user-specified sample rates.
  var supportedSampleRate = function() {
    if(isAudioSupported()) {
      return createAudioContext().sampleRate;
    } else {
      // TODO(ncbray): should the AudioConfig interface be disabled if
      // Audio is not available?
      return 44100;
    }
  }

  var createScriptNode = function(context, sample_count, channels) {
    if (context.createScriptProcessor) {
      return context.createScriptProcessor(sample_count, 0, channels);
    } else {
      // Safari has a differently named API and is buggy if there are zero input channels.
      return context.createJavaScriptNode(sample_count, 1, channels);
    }
  }


  var AudioConfig_CreateStereo16Bit = function(instance, sample_rate, sample_frame_count) {
    if (sample_rate !== supportedSampleRate()) {
      return 0;
    }
    // PP_AUDIOMINSAMPLEFRAMECOUNT = 64
    // PP_AUDIOMAXSAMPLEFRAMECOUNT = 32768
    if (sample_frame_count < 64 || sample_frame_count > 32768) {
      return 0;
    }
    return resources.register(AUDIO_CONFIG_RESOURCE, {
      sample_rate: sample_rate,
      sample_frame_count: sample_frame_count
    });
  };

  var AudioConfig_RecommendSampleFrameCount = function(instance, sample_rate, requested_sample_frame_count) {
    // TODO(ncbray): be smarter?
    return 2048;
  };

  var AudioConfig_IsAudioConfig = function(resource) {
    return resources.is(resource, AUDIO_CONFIG_RESOURCE);
  };

  var AudioConfig_GetSampleRate = function(config) {
    var c = resources.resolve(config, AUDIO_CONFIG_RESOURCE);
    if (c === undefined) {
      return 0;
    }
    return c.sample_rate;
  };

  var AudioConfig_GetSampleFrameCount = function(config) {
    var c = resources.resolve(config, AUDIO_CONFIG_RESOURCE);
    if (c === undefined) {
      return 0;
    }
    return c.sample_frame_count;
  };

  var AudioConfig_RecommendSampleRate = function(instance) {
    return supportedSampleRate();
  };

  registerInterface("PPB_AudioConfig;1.1", [
    AudioConfig_CreateStereo16Bit,
    AudioConfig_RecommendSampleFrameCount,
    AudioConfig_IsAudioConfig,
    AudioConfig_GetSampleRate,
    AudioConfig_GetSampleFrameCount,
    AudioConfig_RecommendSampleRate,
  ]);

  registerInterface("PPB_AudioConfig;1.0", [
    AudioConfig_CreateStereo16Bit,
    AudioConfig_RecommendSampleFrameCount,
    AudioConfig_IsAudioConfig,
    AudioConfig_GetSampleRate,
    AudioConfig_GetSampleFrameCount,
  ]);

  var SetPlaybackState = function(audio, state) {
    if (audio.playing === state) {
      return;
    }
    if (state) {
      audio.processor.connect(audio.context.destination);
      audio.playing = true;
    } else {
      audio.processor.disconnect();
      audio.playing = false;
    }
  };

  var Audio_Create = function(instance, config, audio_callback, user_data) {
    var config_js = resources.resolve(config, AUDIO_CONFIG_RESOURCE);
    if (config_js === undefined) {
      return 0;
    }
    // Assumes 16-bit stereo.
    var buffer_bytes = config_js.sample_frame_count * 2 * 2;
    var buffer = _malloc(buffer_bytes);

    var context = createAudioContext();
    // Note requires power-of-two buffer size.
    var processor = createScriptNode(context, config_js.sample_frame_count, 2);
    processor.onaudioprocess = function (e) {
      Runtime.dynCall('viii', audio_callback, [buffer, buffer_bytes, user_data]);
      var l = e.outputBuffer.getChannelData(0);
      var r = e.outputBuffer.getChannelData(1);
      var base = buffer>>1;
      var offset = 0;
      var scale = 1 / ((1<<15)-1);
      for (var i = 0; i < e.outputBuffer.length; i++) {
        l[i] = HEAP16[base + offset] * scale;
        offset += 1;
        r[i] = HEAP16[base + offset] * scale;
        offset += 1;
      }
    };

    // TODO(ncbray): capture and ref/unref the config?
    // TODO(ncbray): destructor?
    return resources.register(AUDIO_RESOURCE, {
      context: context,
      processor: processor,
      playing: false,
      audio_callback: audio_callback,
      user_data: user_data,
      // Assumes 16-bit stereo.
      buffer: buffer
    });
  };

  var Audio_IsAudio = function(resource) {
    return resources.is(resource, AUDIO_RESOURCE);
  };

  var Audio_GetCurrentConfig = function(audio) {
    throw "Audio_GetCurrentConfig not implemented";
  };

  var Audio_StartPlayback = function(audio) {
    var res = resources.resolve(audio, AUDIO_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    SetPlaybackState(res, true);
    return 1;
  };

  var Audio_StopPlayback = function(audio) {
    var res = resources.resolve(audio, AUDIO_RESOURCE);
    if (res === undefined) {
      return 0;
    }
    SetPlaybackState(res, false);
    return 1;
  };

  registerInterface("PPB_Audio;1.0", [
    Audio_Create,
    Audio_IsAudio,
    Audio_GetCurrentConfig,
    Audio_StartPlayback,
    Audio_StopPlayback,
  ], isAudioSupported);

})();
