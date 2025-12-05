/*
 * Copyright (C) 2011 Hermann Meyer, Andreas Degert
 * Copyright (C) 2025 KAOSS
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 * ---------------------------------------------------------------------------
 *
 *        file: chromasense.cpp      LV2 port of gxtuner
 *
 * ----------------------------------------------------------------------------
 */

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "chromasense_pitch_tracker.h"

#define CHROMASENSE_URI "https://github.com/theKAOSSphere/chromasense"

typedef enum {
    CHROMASENSE_INPUT = 0,
    CHROMASENSE_TUNING,
    CHROMASENSE_NOTE,
    CHROMASENSE_OCTAVE,
    CHROMASENSE_CENT,
    CHROMASENSE_FREQ,
    CHROMASENSE_MUTE,
    CHROMASENSE_OUTPUT
} PortIndex;

typedef struct {
    // Port buffers
    const float* input;
    float*       tuning;
    float*       note;
    float*       octave;
    float*       cent;
    float*       freq;
    float*       mute;
    float*       output;

    // Pitch tracker
    PitchTracker* tracker;
    double        rate;
    float         ref_freq;  // Reference frequency (A4)
    pthread_t     self_thread;

    // Reusable buffer for input copy instead of malloc/free each run
    // This is to guard against memory issues in real-time audio processing
    float*        input_copy;
    uint32_t      max_buffer_size;
} ChromaSense;

static LV2_Handle instantiate(const LV2_Descriptor*     descriptor,
                              double                    rate,
                              const char*               bundle_path,
                              const LV2_Feature* const* features)
{
    ChromaSense* self = (ChromaSense*)calloc(1, sizeof(ChromaSense));
    if (!self) return NULL;
    
    self->rate = rate;
    self->ref_freq = 440.0f;
    self->self_thread = pthread_self();
    
    self->tracker = new PitchTracker();
    self->tracker->init((int)rate, self->self_thread);

    self->max_buffer_size = 8192; // Reasonable maximum buffer size
    self->input_copy = (float*)calloc(self->max_buffer_size, sizeof(float));
    
    return (LV2_Handle)self;
}

static void connect_port(LV2_Handle instance,
                         uint32_t   port,
                         void*      data)
{
    ChromaSense* self = (ChromaSense*)instance;

    switch ((PortIndex)port) {
    case CHROMASENSE_INPUT:
        self->input = (const float*)data;
        break;
    case CHROMASENSE_TUNING:
        self->tuning = (float*)data;
        break;
    case CHROMASENSE_NOTE:
        self->note = (float*)data;
        break;
    case CHROMASENSE_OCTAVE:
        self->octave = (float*)data;
        break;
    case CHROMASENSE_CENT:
        self->cent = (float*)data;
        break;
    case CHROMASENSE_FREQ:
        self->freq = (float*)data;
        break;
    case CHROMASENSE_MUTE:
        self->mute = (float*)data;
        break;
    case CHROMASENSE_OUTPUT:
        self->output = (float*)data;
        break;
    }
}

static void activate(LV2_Handle instance)
{
    ChromaSense* self = (ChromaSense*)instance;
    self->tracker->reset();
}

static void run(LV2_Handle instance, uint32_t n_samples)
{
    ChromaSense* self = (ChromaSense*)instance;
    
    if (!self->input || !self->output) {
        return;
    }

    // Check if muted and handle output first before processing
    const bool is_muted = (self->mute && *(self->mute) > 0.5f);
    
    if (is_muted) {
        // Muted: output silence
        memset(self->output, 0, n_samples * sizeof(float));
    } else {
        // Not muted: pass input to output
        if (self->output != self->input) {
            memmove(self->output, self->input, n_samples * sizeof(float));
        }
    }
    
    // Get reference frequency from input port
    if (self->tuning) {
        float ref_freq = *(self->tuning);
        if (ref_freq != self->ref_freq) {
            self->ref_freq = ref_freq;
        }
    }
    
    // Feed audio to pitch tracker (now using reusable buffer)
    // Also handle case where buffer size > 8192 samples
    if (self->input_copy) {
        uint32_t processed = 0;
        const float* src_ptr = self->input;

        while (processed < n_samples) {
            // Calculate how many samples to grab this time
            uint32_t remaining = n_samples - processed;
            uint32_t chunk_size = (remaining > self->max_buffer_size) ? self->max_buffer_size : remaining;

            // Copy to our safe internal buffer
            memcpy(self->input_copy, src_ptr, chunk_size * sizeof(float));

            // Feed the tracker
            self->tracker->add(chunk_size, self->input_copy);

            // Advance pointers
            src_ptr += chunk_size;
            processed += chunk_size;
        }
    }

    if (self->freq) {
        // Get detected frequency
        float detected_freq = self->tracker->get_estimated_freq();
        *(self->freq) = detected_freq;
        
        if (detected_freq > 0.0f) {
            // Calculate note, octave, and cent deviation
            // Reference: A4 = ref_freq (usually 440 Hz)
            // MIDI note 69 = A4
            // note_num = 12 * log2(freq / ref_freq) + 69

            float note_num = 12.0f * log2f(detected_freq / self->ref_freq) + 69.0f;

            // Round to nearest note
            int nearest_note = (int)roundf(note_num);

            // Note within octave (0-11: C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
            int note_in_octave = nearest_note % 12;
            if (note_in_octave < 0) note_in_octave += 12;

            // Octave (MIDI octave, where C4 = middle C = MIDI 60)
            int octave = (nearest_note / 12) - 1;

            // Cent deviation from nearest note
            float cent = (note_num - (float)nearest_note) * 100.0f;

            *(self->note) = (float)note_in_octave;
            *(self->octave) = (float)octave;
            *(self->cent) = cent;
        } else {
            // No signal detected
            *(self->note) = 0.0f;
            *(self->octave) = 0.0f;
            *(self->cent) = 0.0f;
        }
    }
}

static void deactivate(LV2_Handle instance)
{
}

static void cleanup(LV2_Handle instance)
{
    ChromaSense* self = (ChromaSense*)instance;
    if (self->tracker) {
        self->tracker->stop_thread();
        delete self->tracker;
    }
    if (self->input_copy) {
        free(self->input_copy);
    }
    free(self);
}

static const LV2_Descriptor descriptor = {
    CHROMASENSE_URI,
    instantiate,
    connect_port,
    activate,
    run,
    deactivate,
    cleanup,
    NULL
};

LV2_SYMBOL_EXPORT const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
    switch (index) {
    case 0:  return &descriptor;
    default: return NULL;
    }
}
