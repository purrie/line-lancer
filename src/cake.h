/*
Copyright 2023 Purrie Brightstar

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”),
to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef CAKE_LAYOUT_H
#define CAKE_LAYOUT_H

#ifndef CAKE_RECT
#define CAKE_RECT Rectangle
typedef struct Rectangle Rectangle;
#endif

/*
 * You're expected to define your own rectangle structure. It's supposed to have the following signature:
 * struct Rectangle {
 *     float x, y;
 *     float width, height;
 * };
*/

CAKE_RECT cake_rect             (float width, float height);
CAKE_RECT cake_move_rect        (CAKE_RECT rect, float x, float y);
CAKE_RECT cake_center_rect      (CAKE_RECT rect, float x, float y);
CAKE_RECT cake_margin           (CAKE_RECT rect, float top, float, float bottom, float left, float right);
CAKE_RECT cake_margin_all       (CAKE_RECT rect, float all);
CAKE_RECT cake_cut_horizontal   (CAKE_RECT * rect, float spacing, float ratio);
CAKE_RECT cake_cut_vertical     (CAKE_RECT * rect, float spacing, float ratio);
void      cake_split_horizontal (CAKE_RECT area, float spacing, unsigned int row_count, CAKE_RECT * rows_result);
void      cake_split_grid       (CAKE_RECT area, float spacing, unsigned int cols, unsigned int rows, CAKE_RECT * result);
void      cake_clamp_inside     (CAKE_RECT * area, CAKE_RECT inside);

#ifdef CAKE_LAYOUT_IMPLEMENTATION

Rectangle cake_rect (float width, float height) {
    Rectangle result = { 0.0f, 0.0f, width, height };

    return result;
}

Rectangle cake_move_rect (Rectangle rect, float x, float y) {
    rect.x += x;
    rect.y += y;

    return rect;
}

Rectangle cake_center_rect (Rectangle rect, float x, float y) {
    rect.x = x - rect.width * 0.5f;
    rect.y = y - rect.height * 0.5f;

    return rect;
}

Rectangle cake_margin (Rectangle rect, float top, float, float bottom, float left, float right) {
    rect.x += left;
    rect.y += top;
    rect.width  -= left + right;
    rect.height -= top + bottom;

    return rect;
}

Rectangle cake_margin_all (Rectangle rect, float all) {
    rect.x += all;
    rect.y += all;
    rect.width  -= all * 2.0f;
    rect.height -= all * 2.0f;

    return rect;
}

CAKE_RECT cake_cut_horizontal (CAKE_RECT * rect, float spacing, float ratio) {
    CAKE_RECT result = *rect;

    if (ratio >= 0.0f && ratio <= 1.0f) {
        float spacing_half = spacing * 0.5f;
        result.height = result.height * ratio - spacing_half;
        rect->height = rect->height * (1.0f - ratio) - spacing_half;
        rect->y += result.height + spacing;
    }
    else if (ratio < 0.0f && ratio >= -1.0f) {
        float spacing_half = spacing * 0.5f;
        ratio = -ratio;
        result.height = result.height * (1.0f - ratio) - spacing_half;
        rect->height  = rect->height * ratio - spacing_half;
        result.y += rect->height + spacing;
    }
    else if (ratio > 1.0f && ratio < rect->height) {
        result.height = ratio;
        rect->height -= ratio + spacing;
        rect->y += result.height + spacing;
    }
    else if (ratio < -1.0f && ratio > -rect->height) {
        ratio = -ratio;
        result.height = ratio;
        rect->height -= ratio + spacing;
        result.y += rect->height + spacing;
    }

    return result;
}

CAKE_RECT cake_cut_vertical (CAKE_RECT * rect, float spacing, float ratio) {
    CAKE_RECT result = *rect;

    if (ratio >= 0.0f && ratio <= 1.0f) {
        float spacing_half = spacing * 0.5f;
        result.width = result.width * ratio - spacing_half;
        rect->width = rect->width * (1.0f - ratio) - spacing_half;
        rect->x += result.width + spacing;
    }
    else if (ratio < 0.0f && ratio >= -1.0f) {
        float spacing_half = spacing * 0.5f;
        ratio = -ratio;
        result.width = result.width * (1.0f - ratio) - spacing_half;
        rect->width  = rect->width * ratio - spacing_half;
        result.x += rect->width + spacing;
    }
    else if (ratio > 1.0f && ratio < rect->width) {
        result.width = ratio;
        rect->width -= ratio + spacing;
        rect->x += result.width + spacing;
    }
    else if (ratio < -1.0f && ratio > -rect->width) {
        ratio = -ratio;
        result.width = ratio;
        rect->width -= ratio + spacing;
        result.x += rect->width + spacing;
    }

    return result;
}

void cake_split_horizontal (Rectangle area, float spacing, unsigned int row_count, Rectangle * rows_result) {
    if (row_count == 0) return;
    if (row_count == 1) {
        rows_result[0] = area;
        return;
    }

    float h = area.height / row_count - spacing * ( (row_count - 1) / (float)row_count );

    for (unsigned int i = 0; i < row_count; i++) {
        rows_result[i].x = area.x;
        rows_result[i].width = area.width;

        rows_result[i].y = area.y + h * i + spacing * i;
        rows_result[i].height = h;
    }
}

void cake_split_grid (CAKE_RECT area, float spacing, unsigned int cols, unsigned int rows, CAKE_RECT * result) {
    if (rows == 0 || cols == 0) return;

    float w = area.width  / cols - spacing * ( (cols - 1) / (float)cols );
    float h = area.height / rows - spacing * ( (rows - 1) / (float)rows );

    for (unsigned int y = 0; y < rows; y++) {
        for (unsigned int x = 0; x < cols; x++) {
            unsigned int i = y * cols + x;
            result[i].x = area.x + w * x + spacing * x;
            result[i].y = area.y + h * y + spacing * y;
            result[i].width  = w;
            result[i].height = h;
        }
    }
}

void cake_clamp_inside (CAKE_RECT * area, CAKE_RECT inside) {
    if (area->width > inside.width)
        area->width = inside.width;
    if (area->height > inside.height)
        area->height = inside.height;

    if (area->x < inside.x)
        area->x = inside.x;
    if (area->y < inside.y)
        area->y = inside.y;

    float diff_x = (area->x + area->width) - (inside.x + inside.width);
    float diff_y = (area->y + area->height) - (inside.y + inside.height);

    if (diff_x > 0.0f)
        area->x -= diff_x;
    if (diff_y > 0.0f)
        area->y -= diff_y;
}
#endif

#endif // CAKE_LAYOUT_H
