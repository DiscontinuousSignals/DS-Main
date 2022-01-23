#include "plugin.hpp"
#include "osdialog.h"
#include <dirent.h>

using namespace std;

/**
 * LabelNx Module, allows you to permanently show Text on its front-panel.
 * This is *cf* LABEL/LBL slightly extended.
 */
struct DS_LabelNx : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    std::string label_text = "Right click to write";
    std::string label_align = "left";

    DS_LabelNx() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();
        // lastPath
        json_object_set_new(rootJ, "label_text", json_string(label_text.c_str()));
        json_object_set_new(rootJ, "label_align", json_string(label_align.c_str()));
        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        // lastPath
        json_t *label_text_json = json_object_get(rootJ, "label_text");
        if (label_text_json) {
            label_text = json_string_value(label_text_json);
        }
        json_t *label_align_json = json_object_get(rootJ, "label_align");
        if (label_align_json) {
            label_align = json_string_value(label_align_json);
        }
    }

    /**
     * VCVs process method, nothing is done here (no signal processing).
     * @param args VCV args
     */
    void process(const ProcessArgs &args) override {
        // NOP
    }
};


struct LABELDisplay : TransparentWidget {
    DS_LabelNx *module;

    int frame = 0;

    LABELDisplay() {
    }

    /**
     * Draws the labal text.
     * Considers text from user input or preset as well as text-alignment
     * from preset.
     * @param args VCV DrawAgs
     * @param layer VCV layer num
     */
    void drawLayer(const DrawArgs &args, int layer) override {
        if (layer == 1) {
            shared_ptr <Font> font = APP->window->loadFont(asset::plugin(pluginInstance, "res/LEDCalculator.ttf"));
            std::string to_display = module ? module->label_text : "Right click to write";
            std::string to_align = module ? module->label_align : "left";
            int len = to_display.length();
            if (to_align == "right" && len < 20) {
                int pad = 20 - len;
                to_display.insert(0, pad, ' ');
            } else if (to_align == "center" && len < 20) {
                int pad = (20 - len) / 2;
                to_display.insert(0, pad, ' ');
            }
            nvgFontSize(args.vg, 24);
            nvgFontFaceId(args.vg, font->handle);
            nvgTextLetterSpacing(args.vg, 0);
            nvgFillColor(args.vg, nvgRGBA(0x4c, 0xc7, 0xf3, 0xff));
            nvgRotate(args.vg, -M_PI / 2.0f);
            nvgTextBox(args.vg, 10, 5, 350, to_display.c_str(), NULL);
        }
        Widget::drawLayer(args, layer);
    }
};

struct LABELItem : MenuItem {
    DS_LabelNx *rm;

    /**
     * Set the text to show on label via a user dialog.
     * @param e VCV event-Action
     */
    void onAction(const event::Action &e) override {
        std::string tmp_str = rm ? rm->label_text : "";
        char *chars = osdialog_prompt(OSDIALOG_INFO, "Label :", tmp_str.c_str());
        if (chars) {
            rm->label_text = std::string(chars);
            free(chars);
        }
    }
};

struct LABELWidget : ModuleWidget {
    LABELWidget(DS_LabelNx *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/LabelNx.svg")));

        {
            LABELDisplay *ldisplay = new LABELDisplay();
            ldisplay->box.pos = Vec(18, 333);
            ldisplay->box.size = Vec(130, 250);
            ldisplay->module = module;
            addChild(ldisplay);
        }

        addChild(createWidget<ScrewSilver>(Vec(15, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 30, 365)));
    }

    void appendContextMenu(Menu *menu) override {
        DS_LabelNx *module = dynamic_cast<DS_LabelNx *>(this->module);
        assert(module);

        menu->addChild(new MenuEntry);

        LABELItem *rootDirItem = new LABELItem;
        rootDirItem->text = "Enter label";
        rootDirItem->rm = module;
        menu->addChild(rootDirItem);
    }

};


Model *modelDS_LabelNx = createModel<DS_LabelNx, LABELWidget>("DS_LABELNX");

