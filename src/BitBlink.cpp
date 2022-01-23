#include "plugin.hpp"

struct DS_BitBlink : Module {
    enum ParamId {
        RESET_BTN_PARAM,
        PARAMS_LEN
    };
    enum InputId {
        RESET_IN_INPUT,
        TRIGGER_IN_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        TRIGGER_OUT_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LED00_LIGHT,
        LED01_LIGHT,
        LED02_LIGHT,
        LED03_LIGHT,
        LED04_LIGHT,
        LED05_LIGHT,
        LED06_LIGHT,
        LED07_LIGHT,
        LED08_LIGHT,
        LED09_LIGHT,
        LED10_LIGHT,
        LED11_LIGHT,
        LED12_LIGHT,
        LED13_LIGHT,
        LED14_LIGHT,
        LED15_LIGHT,
        LED16_LIGHT,
        LED17_LIGHT,
        LED18_LIGHT,
        LED19_LIGHT,
        LED20_LIGHT,
        LED21_LIGHT,
        LED22_LIGHT,
        LED23_LIGHT,
        LIGHTS_LEN
    };

    int processEachNFrames = 40; // 48_000 Hz sampling rate divde by eachNFrames
    double outPulseTime = 0.02f; //1e-3;
    int sampleRate;
    int framesPerPulse;
    bool initialized = false;
    // Trigger/Pulses counter;
    unsigned int counter;
    // Trigger DSP object for trig input voltage;
    dsp::SchmittTrigger trigTrigger;
    dsp::SchmittTrigger resetBtnTrigger;
    dsp::SchmittTrigger resetCVInTrigger;
    dsp::PulseGenerator trigPulser;
    long outTriggerHiUntil = -1;

    DS_BitBlink() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        //configParam(TRIGGER_BTN_PARAM, 0.f, 1.f, 0.f, "");
        configButton(RESET_BTN_PARAM, "Reset button");
        //configSwitch(SYNC_PARAM, 0, 1, 0, "Sync mode", {"Soft", "Hard"});
        configInput(RESET_IN_INPUT, "Reset CV");
        configInput(TRIGGER_IN_INPUT, "Trigger CV");
        configOutput(TRIGGER_OUT_OUTPUT, "Trigger CV");

        configLight(LED00_LIGHT, "counting 2^0 = 1");
        configLight(LED01_LIGHT, "counting 2^1 = 2");
        configLight(LED02_LIGHT, "counting 2^2 = 4");
        configLight(LED03_LIGHT, "counting 2^3 = 8");
        configLight(LED04_LIGHT, "counting 2^4 = 16");
        configLight(LED05_LIGHT, "counting 2^5 = 32");
        configLight(LED06_LIGHT, "counting 2^6 = 64");
        configLight(LED07_LIGHT, "counting 2^7 = 128");
        configLight(LED08_LIGHT, "counting 2^8 = 256");
        configLight(LED09_LIGHT, "counting 2^9 = 512");
        configLight(LED10_LIGHT, "counting 2^10 = 1_024");
        configLight(LED11_LIGHT, "counting 2^11 = 2_048");
        configLight(LED12_LIGHT, "counting 2^12 = 4_096");
        configLight(LED13_LIGHT, "counting 2^13 = 8_192");
        configLight(LED14_LIGHT, "counting 2^14 = 16_384");
        configLight(LED15_LIGHT, "counting 2^15 = 32_768");
        configLight(LED16_LIGHT, "counting 2^16 = 65_536");
        configLight(LED17_LIGHT, "counting 2^17 = 131_072");
        configLight(LED18_LIGHT, "counting 2^18 = 262144");
        configLight(LED19_LIGHT, "counting 2^19 = 524288");
        configLight(LED20_LIGHT, "counting 2^20 = 1_048_576");
        configLight(LED21_LIGHT, "counting 2^21 = 2_097_152");
        configLight(LED22_LIGHT, "counting 2^22 = 4_194_304");
        configLight(LED23_LIGHT, "counting 2^23 = 8_388_608");

        framesPerPulse = (int) sampleRate * outPulseTime;
        resetCounter();
    }

    inline void resetCounter() {
        counter = 0;
    }

    inline void resetLeds() {
        for (int i = 0; i < 24; i++) {
            lights[static_cast<LightId>(i)].setBrightness(0.0f);
        }
    }

    inline void updateLeds() {
        for (int i = 0; i < 24; i++) {
            float ledstate = counter & (1 << i) ? 1.f : 0.f;
            lights[static_cast<LightId>(i)].setBrightness(ledstate);
        }
    }

    inline void checkPulseOut(long frameNum) {
        //if (outTriggerHiUntil < 0) {  // output trigegr inactive
        if (lights[LED02_LIGHT].getBrightness() > 0.1f) { // pulse on 2^2 = 4 counter active
            //trigPulser.trigger();
            outTriggerHiUntil = frameNum + framesPerPulse + 1;
            outputs[TRIGGER_OUT_OUTPUT].setVoltage(10.0f);
        }
    }

    inline void checkPulseOutOff(long frameNum) {
        if (outTriggerHiUntil < frameNum && outTriggerHiUntil >= 0) { // switch off pulse
            outputs[TRIGGER_OUT_OUTPUT].setVoltage(0.0f);
            outTriggerHiUntil = -1;
        }
    }

    /**
     * Do audio and CV signal processing.
     * @param args VCV ProcessArgs (Sample Rate and Time, frame-#)
     */
    void process(const ProcessArgs &args) override {
        if (args.frame % processEachNFrames == 0) {  // process only each n-th frame
            if (!initialized) {
                sampleRate = args.sampleRate;
                framesPerPulse = (int) sampleRate * outPulseTime;
                initialized = true;
            }
            if (resetBtnTrigger.process(params[RESET_BTN_PARAM].getValue())
                || resetCVInTrigger.process(inputs[RESET_IN_INPUT].getVoltage())
                ) {
                resetCounter();
                resetLeds();
            }
            if (trigTrigger.process(inputs[TRIGGER_IN_INPUT].getVoltage())) {
                // received a Trigger Pulse
                counter += 1;
                updateLeds();
                checkPulseOut(args.frame);
            }
            if (outTriggerHiUntil >= 0) {
                // check if we have to switch of trigger gate ulse
                checkPulseOutOff(args.frame);
            }
        }
    }
};


struct DS_BitBlinkWidget : ModuleWidget {
    DS_BitBlinkWidget(DS_BitBlink *module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/BitBlink.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        //addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(25.23, 92.614)), module, DS_BitBlink::RESET_BTN_PARAM));
        addParam(createParamCentered<PB61303>(mm2px(Vec(25.23, 92.614)), module, DS_BitBlink::RESET_BTN_PARAM));

        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.8, 92.614)), module, DS_BitBlink::RESET_IN_INPUT));
        addInput(createInputCentered<PJ301MPort>(mm2px(Vec(5.8, 112.0)), module, DS_BitBlink::TRIGGER_IN_INPUT));

        addOutput(
            createOutputCentered<PJ301MPort>(mm2px(Vec(25.2, 112.015)), module, DS_BitBlink::TRIGGER_OUT_OUTPUT));

        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 18.699)), module, DS_BitBlink::LED23_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 22.844)), module, DS_BitBlink::LED22_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 26.515)), module, DS_BitBlink::LED21_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 30.559)), module, DS_BitBlink::LED20_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 34.23)), module, DS_BitBlink::LED19_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 38.375)), module, DS_BitBlink::LED18_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 42.046)), module, DS_BitBlink::LED17_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 45.758)), module, DS_BitBlink::LED16_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 49.429)), module, DS_BitBlink::LED15_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 53.574)), module, DS_BitBlink::LED14_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 57.245)), module, DS_BitBlink::LED13_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 61.289)), module, DS_BitBlink::LED12_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 64.96)), module, DS_BitBlink::LED11_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 69.105)), module, DS_BitBlink::LED10_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 72.775)), module, DS_BitBlink::LED09_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 77.109)), module, DS_BitBlink::LED08_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 80.78)), module, DS_BitBlink::LED07_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 84.925)), module, DS_BitBlink::LED06_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 88.596)), module, DS_BitBlink::LED05_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 92.64)), module, DS_BitBlink::LED04_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 96.311)), module, DS_BitBlink::LED03_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.167, 100.456)), module, DS_BitBlink::LED02_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(18.308, 104.127)), module, DS_BitBlink::LED01_LIGHT));
        addChild(createLightCentered < MediumLight <
                 RedLight >> (mm2px(Vec(11.243, 107.542)), module, DS_BitBlink::LED00_LIGHT));
    }
};


Model *modelDS_BitBlink = createModel<DS_BitBlink, DS_BitBlinkWidget>("DS_BitBlink");