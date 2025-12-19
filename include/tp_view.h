#ifndef TP_VIEW_H
#define TP_VIEW_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TP_View {
    double xmin, xmax;
    double ymin, ymax;
} TP_View;

void tp_view_pan(TP_View *v, double dx, double dy);

/* factor < 1 => zoom in, factor > 1 => zoom out (no centro) */
void tp_view_zoom(TP_View *v, double factor);

/* zoom com âncora (mantém anchor_x/anchor_y “parado” na tela) */
void tp_view_zoom_at(TP_View *v, double factor, double anchor_x, double anchor_y);

#ifdef __cplusplus
}
#endif

#endif
