/*************************************************************************/
/*  default_theme.cpp                                                    */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2018 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2018 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "default_theme.h"

#include "scene/resources/theme.h"

#include "os/os.h"
#include "theme_data.h"

#include "font_hidpi.inc"
#include "font_lodpi.inc"

typedef Map<const void *, Ref<ImageTexture> > TexCacheMap;

static TexCacheMap *tex_cache;
static float scale = 1;

template <class T>
static Ref<StyleBoxTexture> make_stylebox(T p_src, float p_left, float p_top, float p_right, float p_botton, float p_margin_left = -1, float p_margin_top = -1, float p_margin_right = -1, float p_margin_botton = -1, bool p_draw_center = true) {

	Ref<ImageTexture> texture;

	if (tex_cache->has(p_src)) {
		texture = (*tex_cache)[p_src];
	} else {

		texture = Ref<ImageTexture>(memnew(ImageTexture));
		Ref<Image> img = memnew(Image(p_src));

		if (scale > 1) {
			Size2 orig_size = Size2(img->get_width(), img->get_height());

			img->convert(Image::FORMAT_RGBA8);
			img->expand_x2_hq2x();
			if (scale != 2.0) {
				img->resize(orig_size.x * scale, orig_size.y * scale);
			}
		} else if (scale < 1) {
			Size2 orig_size = Size2(img->get_width(), img->get_height());
			img->convert(Image::FORMAT_RGBA8);
			img->resize(orig_size.x * scale, orig_size.y * scale);
		}

		texture->create_from_image(img, ImageTexture::FLAG_FILTER);
		(*tex_cache)[p_src] = texture;
	}

	Ref<StyleBoxTexture> style(memnew(StyleBoxTexture));
	style->set_texture(texture);
	style->set_margin_size(MARGIN_LEFT, p_left * scale);
	style->set_margin_size(MARGIN_RIGHT, p_right * scale);
	style->set_margin_size(MARGIN_BOTTOM, p_botton * scale);
	style->set_margin_size(MARGIN_TOP, p_top * scale);
	style->set_default_margin(MARGIN_LEFT, p_margin_left * scale);
	style->set_default_margin(MARGIN_RIGHT, p_margin_right * scale);
	style->set_default_margin(MARGIN_BOTTOM, p_margin_botton * scale);
	style->set_default_margin(MARGIN_TOP, p_margin_top * scale);
	style->set_draw_center(p_draw_center);

	return style;
}

static Ref<StyleBoxTexture> sb_expand(Ref<StyleBoxTexture> p_sbox, float p_left, float p_top, float p_right, float p_botton) {

	p_sbox->set_expand_margin_size(MARGIN_LEFT, p_left * scale);
	p_sbox->set_expand_margin_size(MARGIN_TOP, p_top * scale);
	p_sbox->set_expand_margin_size(MARGIN_RIGHT, p_right * scale);
	p_sbox->set_expand_margin_size(MARGIN_BOTTOM, p_botton * scale);
	return p_sbox;
}

template <class T>
static Ref<Texture> make_icon(T p_src) {

	Ref<ImageTexture> texture(memnew(ImageTexture));
	Ref<Image> img = memnew(Image(p_src));
	if (scale > 1) {
		Size2 orig_size = Size2(img->get_width(), img->get_height());

		img->convert(Image::FORMAT_RGBA8);
		img->expand_x2_hq2x();
		if (scale != 2.0) {
			img->resize(orig_size.x * scale, orig_size.y * scale);
		}
	} else if (scale < 1) {
		Size2 orig_size = Size2(img->get_width(), img->get_height());
		img->convert(Image::FORMAT_RGBA8);
		img->resize(orig_size.x * scale, orig_size.y * scale);
	}
	texture->create_from_image(img, ImageTexture::FLAG_FILTER);

	return texture;
}

static Ref<Shader> make_shader(const char *vertex_code, const char *fragment_code, const char *lighting_code) {
	Ref<Shader> shader = (memnew(Shader()));
	//shader->set_code(vertex_code, fragment_code, lighting_code);

	return shader;
}

static Ref<BitmapFont> make_font(int p_height, int p_ascent, int p_valign, int p_charcount, const int *p_chars, const Ref<Texture> &p_texture) {

	Ref<BitmapFont> font(memnew(BitmapFont));
	font->add_texture(p_texture);

	for (int i = 0; i < p_charcount; i++) {

		const int *c = &p_chars[i * 8];

		int chr = c[0];
		Rect2 frect;
		frect.position.x = c[1];
		frect.position.y = c[2];
		frect.size.x = c[3];
		frect.size.y = c[4];
		Point2 align(c[5], c[6] + p_valign);
		int advance = c[7];

		font->add_char(chr, 0, frect, align, advance);
	}

	font->set_height(p_height);
	font->set_ascent(p_ascent);

	return font;
}

static Ref<BitmapFont> make_font2(int p_height, int p_ascent, int p_charcount, const int *p_char_rects, int p_kerning_count, const int *p_kernings, int p_w, int p_h, const unsigned char *p_img) {

	Ref<BitmapFont> font(memnew(BitmapFont));

	Ref<Image> image = memnew(Image(p_img));
	Ref<ImageTexture> tex = memnew(ImageTexture);
	tex->create_from_image(image);

	font->add_texture(tex);

	for (int i = 0; i < p_charcount; i++) {

		const int *c = &p_char_rects[i * 8];

		int chr = c[0];
		Rect2 frect;
		frect.position.x = c[1];
		frect.position.y = c[2];
		frect.size.x = c[3];
		frect.size.y = c[4];
		Point2 align(c[6], c[5]);
		int advance = c[7];

		font->add_char(chr, 0, frect, align, advance);
	}

	for (int i = 0; i < p_kerning_count; i++) {

		font->add_kerning_pair(p_kernings[i * 3 + 0], p_kernings[i * 3 + 1], p_kernings[i * 3 + 2]);
	}

	font->set_height(p_height);
	font->set_ascent(p_ascent);

	return font;
}

static Ref<StyleBox> make_empty_stylebox(float p_margin_left = -1, float p_margin_top = -1, float p_margin_right = -1, float p_margin_botton = -1) {

	Ref<StyleBox> style(memnew(StyleBoxEmpty));

	style->set_default_margin(MARGIN_LEFT, p_margin_left * scale);
	style->set_default_margin(MARGIN_RIGHT, p_margin_right * scale);
	style->set_default_margin(MARGIN_BOTTOM, p_margin_botton * scale);
	style->set_default_margin(MARGIN_TOP, p_margin_top * scale);

	return style;
}

void fill_default_theme(Ref<Theme> &theme, const Ref<Font> &default_font, const Ref<Font> &large_font, Ref<Texture> &default_icon, Ref<StyleBox> &default_style, float p_scale) {

	scale = p_scale;

	tex_cache = memnew(TexCacheMap);

	//Ref<BitmapFont> default_font = make_font(_bi_font_normal_height,_bi_font_normal_ascent,_bi_font_normal_valign,_bi_font_normal_charcount,_bi_font_normal_characters,make_icon(font_normal_png));

	// Font Colors

	Color control_font_color = Color::html("e0e0e0");
	Color control_font_color_lower = Color::html("a0a0a0");
	Color control_font_color_low = Color::html("b0b0b0");
	Color control_font_color_hover = Color::html("f0f0f0");
	Color control_font_color_disabled = Color(0.9, 0.9, 0.9, 0.2);
	Color control_font_color_pressed = Color::html("ffffff");
	Color font_color_selection = Color::html("7d7d7d");

	// Panel

	theme->set_stylebox("panel", "Panel", make_stylebox(panel_bg_png, 0, 0, 0, 0));

	// Focus

	Ref<StyleBoxTexture> focus = make_stylebox(focus_png, 5, 5, 5, 5);
	for (int i = 0; i < 4; i++) {
		focus->set_expand_margin_size(Margin(i), 1 * scale);
	}

	// Button

	Ref<StyleBox> sb_button_normal = sb_expand(make_stylebox(button_normal_png, 4, 4, 4, 4, 6, 3, 6, 3), 2, 2, 2, 2);
	Ref<StyleBox> sb_button_pressed = sb_expand(make_stylebox(button_pressed_png, 4, 4, 4, 4, 6, 3, 6, 3), 2, 2, 2, 2);
	Ref<StyleBox> sb_button_hover = sb_expand(make_stylebox(button_hover_png, 4, 4, 4, 4, 6, 2, 6, 2), 2, 2, 2, 2);
	Ref<StyleBox> sb_button_disabled = sb_expand(make_stylebox(button_disabled_png, 4, 4, 4, 4, 6, 2, 6, 2), 2, 2, 2, 2);
	Ref<StyleBox> sb_button_focus = sb_expand(make_stylebox(button_focus_png, 4, 4, 4, 4, 6, 2, 6, 2), 2, 2, 2, 2);

	theme->set_stylebox("normal", "Button", sb_button_normal);
	theme->set_stylebox("pressed", "Button", sb_button_pressed);
	theme->set_stylebox("hover", "Button", sb_button_hover);
	theme->set_stylebox("disabled", "Button", sb_button_disabled);
	theme->set_stylebox("focus", "Button", sb_button_focus);

	theme->set_font("font", "Button", default_font);

	theme->set_color("font_color", "Button", control_font_color);
	theme->set_color("font_color_pressed", "Button", control_font_color_pressed);
	theme->set_color("font_color_hover", "Button", control_font_color_hover);
	theme->set_color("font_color_disabled", "Button", control_font_color_disabled);

	theme->set_constant("hseparation", "Button", 2 * scale);

	// LinkButton

	theme->set_stylebox("focus", "LinkButton", focus);

	theme->set_font("font", "LinkButton", default_font);

	theme->set_color("font_color", "LinkButton", control_font_color);
	theme->set_color("font_color_pressed", "LinkButton", control_font_color_pressed);
	theme->set_color("font_color_hover", "LinkButton", control_font_color_hover);

	theme->set_constant("underline_spacing", "LinkButton", 2 * scale);

	// ColorPickerButton

	theme->set_stylebox("normal", "ColorPickerButton", sb_button_normal);
	theme->set_stylebox("pressed", "ColorPickerButton", sb_button_pressed);
	theme->set_stylebox("hover", "ColorPickerButton", sb_button_hover);
	theme->set_stylebox("disabled", "ColorPickerButton", sb_button_disabled);
	theme->set_stylebox("focus", "ColorPickerButton", sb_button_focus);

	theme->set_font("font", "ColorPickerButton", default_font);

	theme->set_color("font_color", "ColorPickerButton", Color(1, 1, 1, 1));
	theme->set_color("font_color_pressed", "ColorPickerButton", Color(0.8, 0.8, 0.8, 1));
	theme->set_color("font_color_hover", "ColorPickerButton", Color(1, 1, 1, 1));
	theme->set_color("font_color_disabled", "ColorPickerButton", Color(0.9, 0.9, 0.9, 0.3));

	theme->set_constant("hseparation", "ColorPickerButton", 2 * scale);

	// ToolButton

	theme->set_stylebox("normal", "ToolButton", make_empty_stylebox(6, 4, 6, 4));
	theme->set_stylebox("pressed", "ToolButton", make_stylebox(button_pressed_png, 4, 4, 4, 4, 6, 4, 6, 4));
	theme->set_stylebox("hover", "ToolButton", make_stylebox(button_normal_png, 4, 4, 4, 4, 6, 4, 6, 4));
	theme->set_stylebox("disabled", "ToolButton", make_empty_stylebox(6, 4, 6, 4));
	theme->set_stylebox("focus", "ToolButton", focus);
	theme->set_font("font", "ToolButton", default_font);

	theme->set_color("font_color", "ToolButton", control_font_color);
	theme->set_color("font_color_pressed", "ToolButton", control_font_color_pressed);
	theme->set_color("font_color_hover", "ToolButton", control_font_color_hover);
	theme->set_color("font_color_disabled", "ToolButton", Color(0.9, 0.95, 1, 0.3));

	theme->set_constant("hseparation", "ToolButton", 3);

	// OptionButton

	Ref<StyleBox> sb_optbutton_normal = sb_expand(make_stylebox(option_button_normal_png, 4, 4, 21, 4, 6, 3, 21, 3), 2, 2, 2, 2);
	Ref<StyleBox> sb_optbutton_pressed = sb_expand(make_stylebox(option_button_pressed_png, 4, 4, 21, 4, 6, 3, 21, 3), 2, 2, 2, 2);
	Ref<StyleBox> sb_optbutton_hover = sb_expand(make_stylebox(option_button_hover_png, 4, 4, 21, 4, 6, 2, 21, 2), 2, 2, 2, 2);
	Ref<StyleBox> sb_optbutton_disabled = sb_expand(make_stylebox(option_button_disabled_png, 4, 4, 21, 4, 6, 2, 21, 2), 2, 2, 2, 2);
	Ref<StyleBox> sb_optbutton_focus = sb_expand(make_stylebox(button_focus_png, 4, 4, 4, 4, 6, 2, 6, 2), 2, 2, 2, 2);

	theme->set_stylebox("normal", "OptionButton", sb_optbutton_normal);
	theme->set_stylebox("pressed", "OptionButton", sb_optbutton_pressed);
	theme->set_stylebox("hover", "OptionButton", sb_optbutton_hover);
	theme->set_stylebox("disabled", "OptionButton", sb_optbutton_disabled);
	theme->set_stylebox("focus", "OptionButton", sb_optbutton_focus);

	theme->set_icon("arrow", "OptionButton", make_icon(option_arrow_png));

	theme->set_font("font", "OptionButton", default_font);

	theme->set_color("font_color", "OptionButton", control_font_color);
	theme->set_color("font_color_pressed", "OptionButton", control_font_color_pressed);
	theme->set_color("font_color_hover", "OptionButton", control_font_color_hover);
	theme->set_color("font_color_disabled", "OptionButton", control_font_color_disabled);

	theme->set_constant("hseparation", "OptionButton", 2 * scale);
	theme->set_constant("arrow_margin", "OptionButton", 2 * scale);

	// MenuButton

	theme->set_stylebox("normal", "MenuButton", sb_button_normal);
	theme->set_stylebox("pressed", "MenuButton", sb_button_pressed);
	theme->set_stylebox("hover", "MenuButton", sb_button_hover);
	theme->set_stylebox("disabled", "MenuButton", sb_button_disabled);
	theme->set_stylebox("focus", "MenuButton", sb_button_focus);

	theme->set_font("font", "MenuButton", default_font);

	theme->set_color("font_color", "MenuButton", control_font_color);
	theme->set_color("font_color_pressed", "MenuButton", control_font_color_pressed);
	theme->set_color("font_color_hover", "MenuButton", control_font_color_hover);
	theme->set_color("font_color_disabled", "MenuButton", Color(1, 1, 1, 0.3));

	theme->set_constant("hseparation", "MenuButton", 3 * scale);

	// ButtonGroup

	theme->set_stylebox("panel", "ButtonGroup", memnew(StyleBoxEmpty));

	// CheckBox

	Ref<StyleBox> cbx_empty = memnew(StyleBoxEmpty);
	cbx_empty->set_default_margin(MARGIN_LEFT, 4 * scale);
	cbx_empty->set_default_margin(MARGIN_RIGHT, 4 * scale);
	cbx_empty->set_default_margin(MARGIN_TOP, 4 * scale);
	cbx_empty->set_default_margin(MARGIN_BOTTOM, 4 * scale);
	Ref<StyleBox> cbx_focus = focus;
	cbx_focus->set_default_margin(MARGIN_LEFT, 4 * scale);
	cbx_focus->set_default_margin(MARGIN_RIGHT, 4 * scale);
	cbx_focus->set_default_margin(MARGIN_TOP, 4 * scale);
	cbx_focus->set_default_margin(MARGIN_BOTTOM, 4 * scale);

	theme->set_stylebox("normal", "CheckBox", cbx_empty);
	theme->set_stylebox("pressed", "CheckBox", cbx_empty);
	theme->set_stylebox("disabled", "CheckBox", cbx_empty);
	theme->set_stylebox("hover", "CheckBox", cbx_empty);
	theme->set_stylebox("focus", "CheckBox", cbx_focus);

	theme->set_icon("checked", "CheckBox", make_icon(checked_png));
	theme->set_icon("unchecked", "CheckBox", make_icon(unchecked_png));
	theme->set_icon("radio_checked", "CheckBox", make_icon(radio_checked_png));
	theme->set_icon("radio_unchecked", "CheckBox", make_icon(radio_unchecked_png));

	theme->set_font("font", "CheckBox", default_font);

	theme->set_color("font_color", "CheckBox", control_font_color);
	theme->set_color("font_color_pressed", "CheckBox", control_font_color_pressed);
	theme->set_color("font_color_hover", "CheckBox", control_font_color_hover);
	theme->set_color("font_color_disabled", "CheckBox", control_font_color_disabled);

	theme->set_constant("hseparation", "CheckBox", 4 * scale);
	theme->set_constant("check_vadjust", "CheckBox", 0 * scale);

	// CheckButton

	Ref<StyleBox> cb_empty = memnew(StyleBoxEmpty);
	cb_empty->set_default_margin(MARGIN_LEFT, 6 * scale);
	cb_empty->set_default_margin(MARGIN_RIGHT, 6 * scale);
	cb_empty->set_default_margin(MARGIN_TOP, 4 * scale);
	cb_empty->set_default_margin(MARGIN_BOTTOM, 4 * scale);

	theme->set_stylebox("normal", "CheckButton", cb_empty);
	theme->set_stylebox("pressed", "CheckButton", cb_empty);
	theme->set_stylebox("disabled", "CheckButton", cb_empty);
	theme->set_stylebox("hover", "CheckButton", cb_empty);
	theme->set_stylebox("focus", "CheckButton", focus);

	theme->set_icon("on", "CheckButton", make_icon(toggle_on_png));
	theme->set_icon("off", "CheckButton", make_icon(toggle_off_png));

	theme->set_font("font", "CheckButton", default_font);

	theme->set_color("font_color", "CheckButton", control_font_color);
	theme->set_color("font_color_pressed", "CheckButton", control_font_color_pressed);
	theme->set_color("font_color_hover", "CheckButton", control_font_color_hover);
	theme->set_color("font_color_disabled", "CheckButton", control_font_color_disabled);

	theme->set_constant("hseparation", "CheckButton", 4 * scale);
	theme->set_constant("check_vadjust", "CheckButton", 0 * scale);

	// Label

	theme->set_stylebox("normal", "Label", memnew(StyleBoxEmpty));
	theme->set_font("font", "Label", default_font);

	theme->set_color("font_color", "Label", Color(1, 1, 1));
	theme->set_color("font_color_shadow", "Label", Color(0, 0, 0, 0));
	theme->set_color("font_outline_modulate", "Label", Color(1, 1, 1));

	theme->set_constant("shadow_offset_x", "Label", 1 * scale);
	theme->set_constant("shadow_offset_y", "Label", 1 * scale);
	theme->set_constant("shadow_as_outline", "Label", 0 * scale);
	theme->set_constant("line_spacing", "Label", 3 * scale);

	// LineEdit

	theme->set_stylebox("normal", "LineEdit", make_stylebox(line_edit_png, 5, 5, 5, 5));
	theme->set_stylebox("focus", "LineEdit", focus);
	theme->set_stylebox("read_only", "LineEdit", make_stylebox(line_edit_disabled_png, 6, 6, 6, 6));

	theme->set_font("font", "LineEdit", default_font);

	theme->set_color("font_color", "LineEdit", control_font_color);
	theme->set_color("font_color_selected", "LineEdit", Color(0, 0, 0));
	theme->set_color("cursor_color", "LineEdit", control_font_color_hover);
	theme->set_color("selection_color", "LineEdit", font_color_selection);

	theme->set_constant("minimum_spaces", "LineEdit", 12 * scale);

	// ProgressBar

	theme->set_stylebox("bg", "ProgressBar", make_stylebox(progress_bar_png, 4, 4, 4, 4, 0, 0, 0, 0));
	theme->set_stylebox("fg", "ProgressBar", make_stylebox(progress_fill_png, 6, 6, 6, 6, 2, 1, 2, 1));

	theme->set_font("font", "ProgressBar", default_font);

	theme->set_color("font_color", "ProgressBar", control_font_color_hover);
	theme->set_color("font_color_shadow", "ProgressBar", Color(0, 0, 0));

	// TextEdit

	theme->set_stylebox("normal", "TextEdit", make_stylebox(tree_bg_png, 3, 3, 3, 3, 0, 0, 0, 0));
	theme->set_stylebox("focus", "TextEdit", focus);
	theme->set_stylebox("read_only", "TextEdit", make_stylebox(tree_bg_disabled_png, 4, 4, 4, 4, 0, 0, 0, 0));
	theme->set_stylebox("completion", "TextEdit", make_stylebox(tree_bg_png, 3, 3, 3, 3, 0, 0, 0, 0));

	theme->set_icon("tab", "TextEdit", make_icon(tab_png));

	theme->set_font("font", "TextEdit", default_font);

	theme->set_color("background_color", "TextEdit", Color(0, 0, 0, 0));
	theme->set_color("completion_background_color", "TextEdit", Color::html("2C2A32"));
	theme->set_color("completion_selected_color", "TextEdit", Color::html("434244"));
	theme->set_color("completion_existing_color", "TextEdit", Color::html("21dfdfdf"));
	theme->set_color("completion_scroll_color", "TextEdit", control_font_color_pressed);
	theme->set_color("completion_font_color", "TextEdit", Color::html("aaaaaa"));
	theme->set_color("font_color", "TextEdit", control_font_color);
	theme->set_color("font_color_selected", "TextEdit", Color(0, 0, 0));
	theme->set_color("selection_color", "TextEdit", font_color_selection);
	theme->set_color("mark_color", "TextEdit", Color(1.0, 0.4, 0.4, 0.4));
	theme->set_color("breakpoint_color", "TextEdit", Color(0.8, 0.8, 0.4, 0.2));
	theme->set_color("code_folding_color", "TextEdit", Color(0.8, 0.8, 0.8, 0.8));
	theme->set_color("current_line_color", "TextEdit", Color(0.25, 0.25, 0.26, 0.8));
	theme->set_color("caret_color", "TextEdit", control_font_color);
	theme->set_color("caret_background_color", "TextEdit", Color::html("000000"));
	theme->set_color("symbol_color", "TextEdit", control_font_color_hover);
	theme->set_color("brace_mismatch_color", "TextEdit", Color(1, 0.2, 0.2));
	theme->set_color("line_number_color", "TextEdit", Color::html("66aaaaaa"));
	theme->set_color("function_color", "TextEdit", Color::html("66a2ce"));
	theme->set_color("member_variable_color", "TextEdit", Color::html("e64e59"));
	theme->set_color("number_color", "TextEdit", Color::html("EB9532"));
	theme->set_color("word_highlighted_color", "TextEdit", Color(0.8, 0.9, 0.9, 0.15));

	theme->set_constant("completion_lines", "TextEdit", 7);
	theme->set_constant("completion_max_width", "TextEdit", 50);
	theme->set_constant("completion_scroll_width", "TextEdit", 3);
	theme->set_constant("line_spacing", "TextEdit", 4 * scale);

	Ref<Texture> empty_icon = memnew(ImageTexture);

	// HScrollBar

	theme->set_stylebox("scroll", "HScrollBar", make_stylebox(scroll_bg_png, 5, 5, 5, 5, 0, 0, 0, 0));
	theme->set_stylebox("scroll_focus", "HScrollBar", make_stylebox(scroll_bg_png, 5, 5, 5, 5, 0, 0, 0, 0));
	theme->set_stylebox("grabber", "HScrollBar", make_stylebox(scroll_grabber_png, 5, 5, 5, 5, 2, 2, 2, 2));
	theme->set_stylebox("grabber_highlight", "HScrollBar", make_stylebox(scroll_grabber_hl_png, 5, 5, 5, 5, 2, 2, 2, 2));
	theme->set_stylebox("grabber_pressed", "HScrollBar", make_stylebox(scroll_grabber_pressed_png, 5, 5, 5, 5, 2, 2, 2, 2));

	theme->set_icon("increment", "HScrollBar", empty_icon);
	theme->set_icon("increment_highlight", "HScrollBar", empty_icon);
	theme->set_icon("decrement", "HScrollBar", empty_icon);
	theme->set_icon("decrement_highlight", "HScrollBar", empty_icon);

	// VScrollBar

	theme->set_stylebox("scroll", "VScrollBar", make_stylebox(scroll_bg_png, 5, 5, 5, 5, 0, 0, 0, 0));
	theme->set_stylebox("scroll_focus", "VScrollBar", make_stylebox(scroll_bg_png, 5, 5, 5, 5, 0, 0, 0, 0));
	theme->set_stylebox("grabber", "VScrollBar", make_stylebox(scroll_grabber_png, 5, 5, 5, 5, 2, 2, 2, 2));
	theme->set_stylebox("grabber_highlight", "VScrollBar", make_stylebox(scroll_grabber_hl_png, 5, 5, 5, 5, 2, 2, 2, 2));
	theme->set_stylebox("grabber_pressed", "VScrollBar", make_stylebox(scroll_grabber_pressed_png, 5, 5, 5, 5, 2, 2, 2, 2));

	theme->set_icon("increment", "VScrollBar", empty_icon);
	theme->set_icon("increment_highlight", "VScrollBar", empty_icon);
	theme->set_icon("decrement", "VScrollBar", empty_icon);
	theme->set_icon("decrement_highlight", "VScrollBar", empty_icon);

	// HSlider

	theme->set_stylebox("slider", "HSlider", make_stylebox(hslider_bg_png, 4, 4, 4, 4));
	theme->set_stylebox("grabber_area", "HSlider", make_stylebox(hslider_bg_png, 4, 4, 4, 4));
	theme->set_stylebox("grabber_highlight", "HSlider", make_stylebox(hslider_grabber_hl_png, 6, 6, 6, 6));
	theme->set_stylebox("grabber_disabled", "HSlider", make_stylebox(hslider_grabber_disabled_png, 6, 6, 6, 6));
	theme->set_stylebox("focus", "HSlider", focus);

	theme->set_icon("grabber", "HSlider", make_icon(hslider_grabber_png));
	theme->set_icon("grabber_highlight", "HSlider", make_icon(hslider_grabber_hl_png));
	theme->set_icon("grabber_disabled", "HSlider", make_icon(hslider_grabber_disabled_png));
	theme->set_icon("tick", "HSlider", make_icon(hslider_tick_png));

	// VSlider

	theme->set_stylebox("slider", "VSlider", make_stylebox(vslider_bg_png, 4, 4, 4, 4));
	theme->set_stylebox("grabber_area", "VSlider", make_stylebox(vslider_bg_png, 4, 4, 4, 4));
	theme->set_stylebox("grabber_highlight", "VSlider", make_stylebox(vslider_grabber_hl_png, 6, 6, 6, 6));
	theme->set_stylebox("grabber_disabled", "VSlider", make_stylebox(vslider_grabber_disabled_png, 6, 6, 6, 6));
	theme->set_stylebox("focus", "VSlider", focus);

	theme->set_icon("grabber", "VSlider", make_icon(vslider_grabber_png));
	theme->set_icon("grabber_highlight", "VSlider", make_icon(vslider_grabber_hl_png));
	theme->set_icon("grabber_disabled", "VSlider", make_icon(vslider_grabber_disabled_png));
	theme->set_icon("tick", "VSlider", make_icon(vslider_tick_png));

	// SpinBox

	theme->set_icon("updown", "SpinBox", make_icon(spinbox_updown_png));

	//scroll container
	Ref<StyleBoxEmpty> empty;
	empty.instance();
	theme->set_stylebox("bg", "ScrollContainer", empty);

	// WindowDialog

	theme->set_stylebox("panel", "WindowDialog", sb_expand(make_stylebox(popup_window_png, 10, 26, 10, 8), 8, 24, 8, 6));
	theme->set_constant("scaleborder_size", "WindowDialog", 4 * scale);

	theme->set_font("title_font", "WindowDialog", large_font);
	theme->set_color("title_color", "WindowDialog", Color(0, 0, 0));
	theme->set_constant("title_height", "WindowDialog", 20 * scale);

	theme->set_icon("close", "WindowDialog", make_icon(close_png));
	theme->set_icon("close_highlight", "WindowDialog", make_icon(close_hl_png));
	theme->set_constant("close_h_ofs", "WindowDialog", 18 * scale);
	theme->set_constant("close_v_ofs", "WindowDialog", 18 * scale);

	// File Dialog

	theme->set_icon("reload", "FileDialog", make_icon(icon_reload_png));
	theme->set_icon("parent_folder", "FileDialog", make_icon(icon_parent_folder_png));

	// Popup

	Ref<StyleBoxTexture> style_pp = sb_expand(make_stylebox(popup_bg_png, 5, 5, 5, 5, 4, 4, 4, 4), 2, 2, 2, 2);

	Ref<StyleBoxTexture> selected = make_stylebox(selection_png, 6, 6, 6, 6);
	for (int i = 0; i < 4; i++) {
		selected->set_expand_margin_size(Margin(i), 2 * scale);
	}

	theme->set_stylebox("panel", "PopupPanel", style_pp);

	// PopupMenu

	theme->set_stylebox("panel", "PopupMenu", make_stylebox(popup_bg_png, 4, 4, 4, 4, 10, 10, 10, 10));
	theme->set_stylebox("panel_disabled", "PopupMenu", make_stylebox(popup_bg_disabled_png, 4, 4, 4, 4));
	theme->set_stylebox("hover", "PopupMenu", selected);
	theme->set_stylebox("separator", "PopupMenu", make_stylebox(vseparator_png, 3, 3, 3, 3));

	theme->set_icon("checked", "PopupMenu", make_icon(checked_png));
	theme->set_icon("unchecked", "PopupMenu", make_icon(unchecked_png));
	theme->set_icon("radio_checked", "PopupMenu", make_icon(radio_checked_png));
	theme->set_icon("radio_unchecked", "PopupMenu", make_icon(radio_unchecked_png));
	theme->set_icon("submenu", "PopupMenu", make_icon(submenu_png));

	theme->set_font("font", "PopupMenu", default_font);

	theme->set_color("font_color", "PopupMenu", control_font_color);
	theme->set_color("font_color_accel", "PopupMenu", Color(0.7, 0.7, 0.7, 0.8));
	theme->set_color("font_color_disabled", "PopupMenu", Color(0.4, 0.4, 0.4, 0.8));
	theme->set_color("font_color_hover", "PopupMenu", control_font_color);

	theme->set_constant("hseparation", "PopupMenu", 4 * scale);
	theme->set_constant("vseparation", "PopupMenu", 4 * scale);

	// GraphNode

	Ref<StyleBoxTexture> graphsb = make_stylebox(graph_node_png, 6, 24, 6, 5, 16, 24, 16, 5);
	Ref<StyleBoxTexture> graphsbcomment = make_stylebox(graph_node_comment_png, 6, 24, 6, 5, 16, 24, 16, 5);
	Ref<StyleBoxTexture> graphsbcommentselected = make_stylebox(graph_node_comment_focus_png, 6, 24, 6, 5, 16, 24, 16, 5);
	Ref<StyleBoxTexture> graphsbselected = make_stylebox(graph_node_selected_png, 6, 24, 6, 5, 16, 24, 16, 5);
	Ref<StyleBoxTexture> graphsbdefault = make_stylebox(graph_node_default_png, 4, 4, 4, 4, 6, 4, 4, 4);
	Ref<StyleBoxTexture> graphsbdeffocus = make_stylebox(graph_node_default_focus_png, 4, 4, 4, 4, 6, 4, 4, 4);
	Ref<StyleBoxTexture> graph_bpoint = make_stylebox(graph_node_breakpoint_png, 6, 24, 6, 5, 16, 24, 16, 5);
	Ref<StyleBoxTexture> graph_position = make_stylebox(graph_node_position_png, 6, 24, 6, 5, 16, 24, 16, 5);

	//graphsb->set_expand_margin_size(MARGIN_LEFT,10);
	//graphsb->set_expand_margin_size(MARGIN_RIGHT,10);
	theme->set_stylebox("frame", "GraphNode", graphsb);
	theme->set_stylebox("selectedframe", "GraphNode", graphsbselected);
	theme->set_stylebox("defaultframe", "GraphNode", graphsbdefault);
	theme->set_stylebox("defaultfocus", "GraphNode", graphsbdeffocus);
	theme->set_stylebox("comment", "GraphNode", graphsbcomment);
	theme->set_stylebox("commentfocus", "GraphNode", graphsbcommentselected);
	theme->set_stylebox("breakpoint", "GraphNode", graph_bpoint);
	theme->set_stylebox("position", "GraphNode", graph_position);
	theme->set_constant("separation", "GraphNode", 1 * scale);
	theme->set_icon("port", "GraphNode", make_icon(graph_port_png));
	theme->set_icon("close", "GraphNode", make_icon(graph_node_close_png));
	theme->set_icon("resizer", "GraphNode", make_icon(window_resizer_png));
	theme->set_font("title_font", "GraphNode", default_font);
	theme->set_color("title_color", "GraphNode", Color(0, 0, 0, 1));
	theme->set_constant("title_offset", "GraphNode", 20 * scale);
	theme->set_constant("close_offset", "GraphNode", 18 * scale);
	theme->set_constant("port_offset", "GraphNode", 3 * scale);

	// Tree

	Ref<StyleBoxTexture> tree_selected = make_stylebox(selection_png, 4, 4, 4, 4, 8, 0, 8, 0);
	Ref<StyleBoxTexture> tree_selected_oof = make_stylebox(selection_oof_png, 4, 4, 4, 4, 8, 0, 8, 0);

	theme->set_stylebox("bg", "Tree", make_stylebox(tree_bg_png, 4, 4, 4, 5));
	theme->set_stylebox("bg_focus", "Tree", focus);
	theme->set_stylebox("selected", "Tree", tree_selected_oof);
	theme->set_stylebox("selected_focus", "Tree", tree_selected);
	theme->set_stylebox("cursor", "Tree", focus);
	theme->set_stylebox("cursor_unfocused", "Tree", focus);
	theme->set_stylebox("button_pressed", "Tree", make_stylebox(button_pressed_png, 4, 4, 4, 4));
	theme->set_stylebox("title_button_normal", "Tree", make_stylebox(tree_title_png, 4, 4, 4, 4));
	theme->set_stylebox("title_button_pressed", "Tree", make_stylebox(tree_title_pressed_png, 4, 4, 4, 4));
	theme->set_stylebox("title_button_hover", "Tree", make_stylebox(tree_title_png, 4, 4, 4, 4));
	theme->set_stylebox("custom_button", "Tree", sb_button_normal);
	theme->set_stylebox("custom_button_pressed", "Tree", sb_button_pressed);
	theme->set_stylebox("custom_button_hover", "Tree", sb_button_hover);

	theme->set_icon("checked", "Tree", make_icon(checked_png));
	theme->set_icon("unchecked", "Tree", make_icon(unchecked_png));
	theme->set_icon("updown", "Tree", make_icon(updown_png));
	theme->set_icon("select_arrow", "Tree", make_icon(dropdown_png));
	theme->set_icon("arrow", "Tree", make_icon(arrow_down_png));
	theme->set_icon("arrow_collapsed", "Tree", make_icon(arrow_right_png));

	theme->set_font("title_button_font", "Tree", default_font);
	theme->set_font("font", "Tree", default_font);

	theme->set_color("title_button_color", "Tree", control_font_color);
	theme->set_color("font_color", "Tree", control_font_color_low);
	theme->set_color("font_color_selected", "Tree", control_font_color_pressed);
	theme->set_color("selection_color", "Tree", Color(0.1, 0.1, 1, 0.8));
	theme->set_color("cursor_color", "Tree", Color(0, 0, 0));
	theme->set_color("guide_color", "Tree", Color(0, 0, 0, 0.1));
	theme->set_color("drop_position_color", "Tree", Color(1, 0.3, 0.2));
	theme->set_color("relationship_line_color", "Tree", Color::html("464646"));
	theme->set_color("custom_button_font_highlight", "Tree", control_font_color_hover);

	theme->set_constant("hseparation", "Tree", 4 * scale);
	theme->set_constant("vseparation", "Tree", 4 * scale);
	theme->set_constant("guide_width", "Tree", 2 * scale);
	theme->set_constant("item_margin", "Tree", 12 * scale);
	theme->set_constant("button_margin", "Tree", 4 * scale);
	theme->set_constant("draw_relationship_lines", "Tree", 0);
	theme->set_constant("scroll_border", "Tree", 4);
	theme->set_constant("scroll_speed", "Tree", 12);

	// ItemList
	Ref<StyleBoxTexture> item_selected = make_stylebox(selection_png, 4, 4, 4, 4, 8, 2, 8, 2);
	Ref<StyleBoxTexture> item_selected_oof = make_stylebox(selection_oof_png, 4, 4, 4, 4, 8, 2, 8, 2);

	theme->set_stylebox("bg", "ItemList", make_stylebox(tree_bg_png, 4, 4, 4, 5));
	theme->set_stylebox("bg_focus", "ItemList", focus);
	theme->set_constant("hseparation", "ItemList", 4);
	theme->set_constant("vseparation", "ItemList", 2);
	theme->set_constant("icon_margin", "ItemList", 4);
	theme->set_constant("line_separation", "ItemList", 2 * scale);
	theme->set_font("font", "ItemList", default_font);
	theme->set_color("font_color", "ItemList", control_font_color_lower);
	theme->set_color("font_color_selected", "ItemList", control_font_color_pressed);
	theme->set_color("guide_color", "ItemList", Color(0, 0, 0, 0.1));
	theme->set_stylebox("selected", "ItemList", item_selected_oof);
	theme->set_stylebox("selected_focus", "ItemList", item_selected);
	theme->set_stylebox("cursor", "ItemList", focus);
	theme->set_stylebox("cursor_unfocused", "ItemList", focus);

	// TabContainer

	Ref<StyleBoxTexture> tc_sb = sb_expand(make_stylebox(tab_container_bg_png, 4, 4, 4, 4, 4, 4, 4, 4), 3, 3, 3, 3);

	tc_sb->set_expand_margin_size(MARGIN_TOP, 2 * scale);
	tc_sb->set_default_margin(MARGIN_TOP, 8 * scale);

	theme->set_stylebox("tab_fg", "TabContainer", sb_expand(make_stylebox(tab_current_png, 4, 4, 4, 1, 16, 4, 16, 4), 2, 2, 2, 2));
	theme->set_stylebox("tab_bg", "TabContainer", sb_expand(make_stylebox(tab_behind_png, 5, 5, 5, 1, 16, 6, 16, 4), 3, 0, 3, 3));
	theme->set_stylebox("panel", "TabContainer", tc_sb);

	theme->set_icon("increment", "TabContainer", make_icon(scroll_button_right_png));
	theme->set_icon("increment_highlight", "TabContainer", make_icon(scroll_button_right_hl_png));
	theme->set_icon("decrement", "TabContainer", make_icon(scroll_button_left_png));
	theme->set_icon("decrement_highlight", "TabContainer", make_icon(scroll_button_left_hl_png));
	theme->set_icon("menu", "TabContainer", make_icon(tab_menu_png));
	theme->set_icon("menu_highlight", "TabContainer", make_icon(tab_menu_hl_png));

	theme->set_font("font", "TabContainer", default_font);

	theme->set_color("font_color_fg", "TabContainer", control_font_color_hover);
	theme->set_color("font_color_bg", "TabContainer", control_font_color_low);
	theme->set_color("font_color_disabled", "TabContainer", control_font_color_disabled);

	theme->set_constant("side_margin", "TabContainer", 8 * scale);
	theme->set_constant("top_margin", "TabContainer", 24 * scale);
	theme->set_constant("label_valign_fg", "TabContainer", 0 * scale);
	theme->set_constant("label_valign_bg", "TabContainer", 2 * scale);
	theme->set_constant("hseparation", "TabContainer", 4 * scale);

	// Tabs

	theme->set_stylebox("tab_fg", "Tabs", sb_expand(make_stylebox(tab_current_png, 4, 3, 4, 1, 16, 3, 16, 2), 2, 2, 2, 2));
	theme->set_stylebox("tab_bg", "Tabs", sb_expand(make_stylebox(tab_behind_png, 5, 4, 5, 1, 16, 5, 16, 2), 3, 3, 3, 3));
	theme->set_stylebox("panel", "Tabs", tc_sb);
	theme->set_stylebox("button_pressed", "Tabs", make_stylebox(button_pressed_png, 4, 4, 4, 4));
	theme->set_stylebox("button", "Tabs", make_stylebox(button_normal_png, 4, 4, 4, 4));

	theme->set_icon("increment", "Tabs", make_icon(scroll_button_right_png));
	theme->set_icon("increment_highlight", "Tabs", make_icon(scroll_button_right_hl_png));
	theme->set_icon("decrement", "Tabs", make_icon(scroll_button_left_png));
	theme->set_icon("decrement_highlight", "Tabs", make_icon(scroll_button_left_hl_png));
	theme->set_icon("close", "Tabs", make_icon(tab_close_png));

	theme->set_font("font", "Tabs", default_font);

	theme->set_color("font_color_fg", "Tabs", control_font_color_hover);
	theme->set_color("font_color_bg", "Tabs", control_font_color_low);
	theme->set_color("font_color_disabled", "Tabs", control_font_color_disabled);

	theme->set_constant("top_margin", "Tabs", 24 * scale);
	theme->set_constant("label_valign_fg", "Tabs", 0 * scale);
	theme->set_constant("label_valign_bg", "Tabs", 2 * scale);
	theme->set_constant("hseparation", "Tabs", 4 * scale);

	// Separators

	theme->set_stylebox("separator", "HSeparator", make_stylebox(vseparator_png, 3, 3, 3, 3));
	theme->set_stylebox("separator", "VSeparator", make_stylebox(hseparator_png, 3, 3, 3, 3));

	theme->set_icon("close", "Icons", make_icon(icon_close_png));
	theme->set_font("normal", "Fonts", default_font);
	theme->set_font("large", "Fonts", large_font);

	theme->set_constant("separation", "HSeparator", 4 * scale);
	theme->set_constant("separation", "VSeparator", 4 * scale);

	// Dialogs

	theme->set_constant("margin", "Dialogs", 8 * scale);
	theme->set_constant("button_margin", "Dialogs", 32 * scale);

	// FileDialog

	theme->set_icon("folder", "FileDialog", make_icon(icon_folder_png));
	theme->set_color("files_disabled", "FileDialog", Color(0, 0, 0, 0.7));

	// colorPicker

	theme->set_constant("margin", "ColorPicker", 4 * scale);
	theme->set_constant("sv_width", "ColorPicker", 256 * scale);
	theme->set_constant("sv_height", "ColorPicker", 256 * scale);
	theme->set_constant("h_width", "ColorPicker", 30 * scale);
	theme->set_constant("label_width", "ColorPicker", 10 * scale);

	theme->set_icon("screen_picker", "ColorPicker", make_icon(icon_color_pick_png));
	theme->set_icon("add_preset", "ColorPicker", make_icon(icon_add_png));
	theme->set_icon("color_hue", "ColorPicker", make_icon(color_picker_hue_png));
	theme->set_icon("color_sample", "ColorPicker", make_icon(color_picker_sample_png));
	theme->set_icon("preset_bg", "ColorPicker", make_icon(mini_checkerboard_png));

	theme->set_icon("bg", "ColorPickerButton", make_icon(mini_checkerboard_png));

	// TooltipPanel

	Ref<StyleBoxTexture> style_tt = make_stylebox(tooltip_bg_png, 4, 4, 4, 4);
	for (int i = 0; i < 4; i++)
		style_tt->set_expand_margin_size((Margin)i, 4 * scale);

	theme->set_stylebox("panel", "TooltipPanel", style_tt);

	theme->set_font("font", "TooltipLabel", default_font);

	theme->set_color("font_color", "TooltipLabel", Color(0, 0, 0));
	theme->set_color("font_color_shadow", "TooltipLabel", Color(0, 0, 0, 0.1));

	theme->set_constant("shadow_offset_x", "TooltipLabel", 1);
	theme->set_constant("shadow_offset_y", "TooltipLabel", 1);

	// RichTextLabel

	theme->set_stylebox("focus", "RichTextLabel", focus);
	theme->set_stylebox("normal", "RichTextLabel", make_empty_stylebox(0, 0, 0, 0));

	theme->set_font("normal_font", "RichTextLabel", default_font);
	theme->set_font("bold_font", "RichTextLabel", default_font);
	theme->set_font("italics_font", "RichTextLabel", default_font);
	theme->set_font("bold_italics_font", "RichTextLabel", default_font);
	theme->set_font("mono_font", "RichTextLabel", default_font);

	theme->set_color("default_color", "RichTextLabel", control_font_color);
	theme->set_color("font_color_selected", "RichTextLabel", font_color_selection);
	theme->set_color("selection_color", "RichTextLabel", Color(0.1, 0.1, 1, 0.8));

	theme->set_color("font_color_shadow", "RichTextLabel", Color(0, 0, 0, 0));

	theme->set_constant("shadow_offset_x", "RichTextLabel", 1 * scale);
	theme->set_constant("shadow_offset_y", "RichTextLabel", 1 * scale);
	theme->set_constant("shadow_as_outline", "RichTextLabel", 0 * scale);

	theme->set_constant("line_separation", "RichTextLabel", 1 * scale);
	theme->set_constant("table_hseparation", "RichTextLabel", 3 * scale);
	theme->set_constant("table_vseparation", "RichTextLabel", 3 * scale);

	// Containers

	theme->set_stylebox("bg", "VSplitContainer", make_stylebox(vsplit_bg_png, 1, 1, 1, 1));
	theme->set_stylebox("bg", "HSplitContainer", make_stylebox(hsplit_bg_png, 1, 1, 1, 1));

	theme->set_icon("grabber", "VSplitContainer", make_icon(vsplitter_png));
	theme->set_icon("grabber", "HSplitContainer", make_icon(hsplitter_png));

	theme->set_constant("separation", "HBoxContainer", 4 * scale);
	theme->set_constant("separation", "VBoxContainer", 4 * scale);
	theme->set_constant("margin_left", "MarginContainer", 0 * scale);
	theme->set_constant("margin_top", "MarginContainer", 0 * scale);
	theme->set_constant("margin_right", "MarginContainer", 0 * scale);
	theme->set_constant("margin_bottom", "MarginContainer", 0 * scale);
	theme->set_constant("hseparation", "GridContainer", 4 * scale);
	theme->set_constant("vseparation", "GridContainer", 4 * scale);
	theme->set_constant("separation", "HSplitContainer", 12 * scale);
	theme->set_constant("separation", "VSplitContainer", 12 * scale);
	theme->set_constant("autohide", "HSplitContainer", 1 * scale);
	theme->set_constant("autohide", "VSplitContainer", 1 * scale);

	// ReferenceRect

	Ref<StyleBoxTexture> ttnc = make_stylebox(full_panel_bg_png, 8, 8, 8, 8);
	ttnc->set_draw_center(false);

	theme->set_stylebox("border", "ReferenceRect", make_stylebox(reference_border_png, 4, 4, 4, 4));
	theme->set_stylebox("panelnc", "Panel", ttnc);
	theme->set_stylebox("panelf", "Panel", tc_sb);

	Ref<StyleBoxTexture> sb_pc = make_stylebox(tab_container_bg_png, 4, 4, 4, 4, 7, 7, 7, 7);
	theme->set_stylebox("panel", "PanelContainer", sb_pc);

	theme->set_icon("minus", "GraphEdit", make_icon(icon_zoom_less_png));
	theme->set_icon("reset", "GraphEdit", make_icon(icon_zoom_reset_png));
	theme->set_icon("more", "GraphEdit", make_icon(icon_zoom_more_png));
	theme->set_icon("snap", "GraphEdit", make_icon(icon_snap_grid_png));
	theme->set_stylebox("bg", "GraphEdit", make_stylebox(tree_bg_png, 4, 4, 4, 5));
	theme->set_color("grid_minor", "GraphEdit", Color(1, 1, 1, 0.05));
	theme->set_color("grid_major", "GraphEdit", Color(1, 1, 1, 0.2));
	theme->set_color("activity", "GraphEdit", Color(1, 1, 1));
	theme->set_constant("bezier_len_pos", "GraphEdit", 80 * scale);
	theme->set_constant("bezier_len_neg", "GraphEdit", 160 * scale);

	theme->set_icon("logo", "Icons", make_icon(logo_png));

	// Theme

	default_icon = make_icon(error_icon_png);
	default_style = make_stylebox(error_icon_png, 2, 2, 2, 2);

	memdelete(tex_cache);
}

void make_default_theme(bool p_hidpi, Ref<Font> p_font) {

	Ref<Theme> t;
	t.instance();

	Ref<StyleBox> default_style;
	Ref<Texture> default_icon;
	Ref<Font> default_font;
	if (p_font.is_valid()) {
		default_font = p_font;
	} else if (p_hidpi) {
		default_font = make_font2(_hidpi_font_height, _hidpi_font_ascent, _hidpi_font_charcount, &_hidpi_font_charrects[0][0], _hidpi_font_kerning_pair_count, &_hidpi_font_kerning_pairs[0][0], _hidpi_font_img_width, _hidpi_font_img_height, _hidpi_font_img_data);
	} else {
		default_font = make_font2(_lodpi_font_height, _lodpi_font_ascent, _lodpi_font_charcount, &_lodpi_font_charrects[0][0], _lodpi_font_kerning_pair_count, &_lodpi_font_kerning_pairs[0][0], _lodpi_font_img_width, _lodpi_font_img_height, _lodpi_font_img_data);
	}
	Ref<Font> large_font = default_font;
	fill_default_theme(t, default_font, large_font, default_icon, default_style, p_hidpi ? 2.0 : 1.0);

	Theme::set_default(t);
	Theme::set_default_icon(default_icon);
	Theme::set_default_style(default_style);
	Theme::set_default_font(default_font);
}

void clear_default_theme() {

	Theme::set_default(Ref<Theme>());
	Theme::set_default_icon(Ref<Texture>());
	Theme::set_default_style(Ref<StyleBox>());
	Theme::set_default_font(Ref<Font>());
}