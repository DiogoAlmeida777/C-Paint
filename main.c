#include "include/raylib.h"
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "include/raygui.h"
// #include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "include/raymath.h"

#define MAX_COLORS_COUNT 42
#define MAX_TOOLS_COUNT 12
#define MAX_BRUSH_MODES_COUNT 2
#define RESIZE_SQUARE_SIDE_SIZE 5


#define MENU_GRAY (Color){225,225,225,255}



// ENUMS

typedef enum {
    BRUSH = 0,
    ERASER,
    AIR_BRUSH,
    COLOR_BUCKET,
    COLOR_PICKER,
    TEXT_BOX,
    MAGNIFIER,
    LINE,
    CURVE,
    RECTANGLE,
    OVAL,
    POLYGON
} Tools;

typedef enum {
    ROUND = 0,
    SQUARE = 1,
} BrushMode;

typedef enum{
    IDLE,
    MAKING_LINE,
    BENDING
} SplineState;

// STRUCTS

typedef struct S_Brush {
    float size;
    BrushMode mode;
    int brushModeIndex;
} Brush;

typedef struct S_AirBrush{
    float radius;
    float spray_rate;
} AirBrush;

typedef struct S_Shape {
    float outline_size;
    bool has_outline;
    bool is_filled;
} Shape;

typedef struct S_Text{
    char *buffer;
    int text_length;
    int buffer_size;
    int font_size;
    Vector2 pos;
    bool is_writing;
    int line_count;
    int last_newline_index;
} Text;

typedef struct S_Polygon
{
    int maxY;
    int minY;
    int num_of_vertices;
    int capacity;
    Vector2 *vertices;
    float outline_size;
    bool has_outline;
    bool is_filled;

} Polygon;

typedef struct S_Spline{
    int max_points;
    Vector2 *points;
    float thickness;
    SplineState state;
    int index;
} Spline;

typedef void (*drawFunc)(Vector2*,Vector2*,Color,Color,Shape);

typedef void (*GUIFunc)(void *, Rectangle);

// CONSTRUCTORS

Brush *brush(int size, BrushMode mode)
{
    Brush *brush;
    brush = malloc(sizeof(Brush));
    brush->size = size;
    brush->mode = mode;
    brush->brushModeIndex = (int)mode;
    return brush;
}

AirBrush *airbrush(int radius, float spray_rate)
{
    AirBrush *airbrush;
    airbrush = malloc(sizeof(AirBrush));
    airbrush->radius = radius;
    airbrush->spray_rate = spray_rate;
    return airbrush;
}

Shape *shape(int size, bool outline, bool fill)
{
    Shape *shape;
    shape = malloc(sizeof(Shape));
    if(!outline && !fill){
        fprintf(stderr, "Error: Shape must have atleast outline or fill equal true.\n");
        exit(EXIT_FAILURE);
    }
    shape->outline_size = size;
    shape->has_outline = outline;
    shape->is_filled = fill;
    return shape;
}

void createNewTextBuffer(Text *text)
{
    if(text->buffer != NULL)
        free(text->buffer);
    text->buffer_size = 64;
    text->buffer = malloc(text->buffer_size);
    if(!text->buffer){
        free(text);
        fprintf(stderr, "Error: failed to allocate memory to text buffer.\n");
        return;
    }
    text->buffer[0] = '\0';
    text->text_length = 0;
    text->line_count = 0;
    text->last_newline_index = 0;
}


Text *text(int font_size){
    Text *text;
    text = malloc(sizeof(Text));
    if(!text){
        fprintf(stderr, "Error: failed to allocate memory to text struct.\n");
        return NULL;
    }
    text->buffer = NULL;
    createNewTextBuffer(text);
    text->font_size = font_size;
    text->pos = (Vector2){0,0};
    text->is_writing = false;
    return text;
}

void createNewVertices(Polygon *polygon){
    if(polygon->vertices != NULL)
        free(polygon->vertices);
    polygon->capacity = 32;
    polygon->num_of_vertices = 0;
    polygon->maxY = -1;
    polygon->minY = INT_MAX;
    polygon->vertices = malloc(sizeof(Vector2) * polygon->capacity);
    if (!polygon->vertices) {
        fprintf(stderr, "Error: failed to allocate memory for vertices.\n");
        free(polygon);
        exit(EXIT_FAILURE);
    }
}

Polygon *polygon(int outline_size, bool outline, bool fill)
{
    if(!outline && !fill){
        fprintf(stderr, "Error: Shape must have atleast outline or fill equal true.\n");
        exit(EXIT_FAILURE);
    }
    Polygon *polygon;
    polygon = malloc(sizeof(Polygon));
    if (!polygon) {
        fprintf(stderr, "Error: failed to allocate memory for Polygon.\n");
        exit(EXIT_FAILURE);
    }
    createNewVertices(polygon);
    polygon->outline_size = outline_size;
    polygon->has_outline = outline;
    polygon->is_filled = fill;
    return polygon;                                                                                                                                  
}

void createNewPoints(Spline *spline){
    if(spline->points != NULL){
        free(spline->points);
    }
    spline->points = malloc(sizeof(Vector2)*spline->max_points);
    if (!spline->points) {
        fprintf(stderr, "Error: failed to allocate memory for spline's points.\n");
        free(spline);
        exit(EXIT_FAILURE);
    }
}

Spline *spline(int max_points,int thickness){
    if(max_points < 4 ){
        fprintf(stderr, "Error: spline must have atleast 4 points.\n");
        exit(EXIT_FAILURE);
    }
    Spline *spline;
    spline = malloc(sizeof(Spline));
    if(!spline){
        fprintf(stderr, "Error: failed to allocate memory for Spline.\n");
        exit(EXIT_FAILURE);
    }
    spline->max_points = max_points;
    spline->thickness = thickness;
    spline->state = IDLE;
    spline->index = 0;
    createNewPoints(spline);
    return spline;
}

// FREE FUNCTIONS

void freeBrush(Brush *brush){
    free(brush);
}

void freeAirBrush(AirBrush *airbrush)
{
    free(airbrush);
}

void freeShape(Shape *shape){
    free(shape);
}

void freeText(Text *text){
    free(text->buffer);
    free(text);
}

void freePolygon(Polygon *polygon){
    free(polygon->vertices);
    free(polygon);
}

void freeSpline(Spline *spline){
    free(spline->points);
    free(spline);
}


void addVertexToPolygon(Polygon *polygon, Vector2 v){
    if(polygon->num_of_vertices >= polygon->capacity){
        polygon->capacity *= 2;
        Vector2 *new_vertices = realloc(polygon->vertices,sizeof(Vector2) * polygon->capacity);
        if (!new_vertices) {
            fprintf(stderr, "Error: failed to reallocate memory for vertices.\n");
            return;
        }
        polygon->vertices = new_vertices;
    }
    
    polygon->vertices[polygon->num_of_vertices++] = v;

    if (v.y > polygon->maxY) polygon->maxY = v.y;
    if (v.y < polygon->minY) polygon->minY = v.y;
}

float distanceBetweenVectors(Vector2 v1, Vector2 v2){
    float dx = v2.x - v1.x;
    float dy = v2.y - v1.y;
    return sqrtf(dx * dx + dy * dy);
}


int changeSize(int actualSize, int increment)
{
    int newSize = actualSize + increment ;
    if(newSize >= 1 && newSize <= 248)
        actualSize = newSize;
    return actualSize;
}

//BRUSH AND ERASER FUNCTIONS

void brushDraw(Vector2 mouse, Vector2 lastMouse, int brushSize, BrushMode paintMode, Color color){
    
    if (lastMouse.x != -1 && lastMouse.y != -1)
    {
        float dx = mouse.x - lastMouse.x;
        float dy = mouse.y - lastMouse.y;
        float distance = sqrtf(dx*dx + dy*dy);

        int steps = (int)distance;
        for (int i = 0; i < steps; i++)
        {
            float t = (float)i / distance;
            Vector2 interp = {
                lastMouse.x + t * dx,
                lastMouse.y + t * dy
            };
            if (paintMode == ROUND)
                DrawCircleV(interp, brushSize, color);
            else
                DrawRectangle(interp.x,interp.y,brushSize,brushSize,color);
        }
    }

    if (paintMode == ROUND)
        DrawCircleV(mouse, brushSize, color);
    else
        DrawRectangle(mouse.x,mouse.y,brushSize,brushSize,color);

}

void paint(RenderTexture2D *canvas, Vector2 *mouseInCanvas, Vector2 *lastMouse, Brush *tool, Color color)
{
    BeginTextureMode(*canvas);

    brushDraw(*mouseInCanvas,*lastMouse,tool->size,tool->mode,color);
    *lastMouse = *mouseInCanvas;
    
    EndTextureMode();

}

// SHAPES FUNCTIONS

Vector2 getTopLeft(Vector2 *lastMouse, Vector2 *mouseInCanvas)
{
    Vector2 topleft = {lastMouse->x,lastMouse->y};

    if(mouseInCanvas->x < lastMouse->x)
            topleft.x = mouseInCanvas->x;
                    
    if(mouseInCanvas->y < lastMouse->y)
        topleft.y = mouseInCanvas->y;
    
    return topleft;
}

void drawRec(Vector2 *lastMouse, Vector2 *mouseInCanvas,Color fill_color, Color outline_color, Shape recInfo){
    Vector2 topleft = getTopLeft(lastMouse,mouseInCanvas);
    Rectangle rec = {topleft.x,topleft.y,abs(mouseInCanvas->x-lastMouse->x),abs(mouseInCanvas->y - lastMouse->y)};

    if(recInfo.is_filled)
        DrawRectangleRec(rec,fill_color);
    if(recInfo.has_outline)
        DrawRectangleLinesEx(rec,recInfo.outline_size,outline_color);


}

void drawOval(Vector2 *lastMouse, Vector2 *mouseInCanvas,Color fill_color, Color outline_color, Shape ovalInfo)
{
    Vector2 topleft = getTopLeft(lastMouse,mouseInCanvas);
    float radiusH =  abs(mouseInCanvas->x-lastMouse->x) /2;
    float radiusV = abs(mouseInCanvas->y - lastMouse->y) / 2;
    int centerX = topleft.x + radiusH;
    int centerY = topleft.y + radiusV;

    if(ovalInfo.has_outline)
        DrawEllipseLines(centerX,centerY,radiusH,radiusV,outline_color);
    if(ovalInfo.is_filled)
        DrawEllipse(centerX,centerY,radiusH,radiusV,fill_color);
                    
}

void drawShape(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview, Vector2 *lastMouse, Vector2 *mouseInCanvas,Color fill_color, Color outline_color,Shape shapeInfo, drawFunc draw_func, bool isMouseOverCanvas){
    if(IsMouseButtonPressed(mouse_button) && isMouseOverCanvas)
    {
        *lastMouse = *mouseInCanvas;
    }
    else if(IsMouseButtonDown(mouse_button) && !Vector2Equals(*lastMouse,(Vector2){-1,-1}))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        if(draw_func != NULL)
        {
            draw_func(lastMouse,mouseInCanvas,fill_color,outline_color,shapeInfo);
        }
        EndTextureMode();
    }
    else if(IsMouseButtonReleased(mouse_button) && !Vector2Equals(*lastMouse,(Vector2){-1,-1}))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        EndTextureMode();
        BeginTextureMode(*canvas);
        if(draw_func != NULL)
        {
            draw_func(lastMouse,mouseInCanvas,fill_color,outline_color,shapeInfo);
        }
        EndTextureMode();
        lastMouse->x = -1;
        lastMouse->y = -1;
    }

}

void drawLine(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview, Vector2 *lastMouse, Vector2 *mouseInCanvas,Color color,int lineSize, bool isMouseOverCanvas){
    if(IsMouseButtonPressed(mouse_button) && isMouseOverCanvas)
    {
        *lastMouse = *mouseInCanvas;
    }
    else if(IsMouseButtonDown(mouse_button) && !Vector2Equals(*lastMouse,(Vector2){-1,-1}))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        DrawCircle(lastMouse->x,lastMouse->y,lineSize/2,color);
        DrawLineEx(*lastMouse,*mouseInCanvas,lineSize,color);
        DrawCircle(mouseInCanvas->x,mouseInCanvas->y,lineSize/2,color);
        EndTextureMode();
    }
    else if(IsMouseButtonReleased(mouse_button) && !Vector2Equals(*lastMouse,(Vector2){-1,-1}))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        EndTextureMode();
        BeginTextureMode(*canvas);
        DrawCircle(lastMouse->x,lastMouse->y,lineSize/2,color);
        DrawLineEx(*lastMouse,*mouseInCanvas,lineSize,color);
        DrawCircle(mouseInCanvas->x,mouseInCanvas->y,lineSize/2,color);
        EndTextureMode();
        lastMouse->x = -1;
        lastMouse->y = -1;
        
    }

}

void setAndDrawSplinePoints(Spline *spline,RenderTexture2D *preview,Vector2 *mouseInCanvas,Color color){
    spline->points[2] = (Vector2){mouseInCanvas->x,mouseInCanvas->y};
    int x1 = spline->points[1].x - 0.25f * (spline->points[2].x -spline->points[1].x);
    int y1 = spline->points[1].y - 0.25f * (spline->points[2].y -spline->points[1].y);
    int x2 = spline->points[2].x + 0.25f * (spline->points[2].x -spline->points[1].x);
    int y2 = spline->points[2].y + 0.25f * (spline->points[2].y -spline->points[1].y);
    spline->points[0] = (Vector2){x1,y1};
    spline->points[3] = (Vector2){x2,y2};
    BeginTextureMode(*preview);
    ClearBackground(BLANK);
    DrawSplineCatmullRom(spline->points,spline->max_points,spline->thickness, color);
    EndTextureMode();
}

void drawSpline(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview,Vector2 *lastMouse,Vector2 *mouseInCanvas,Color color,Spline *spline,bool isMouseOverCanvas){
    if(IsMouseButtonPressed(mouse_button) && isMouseOverCanvas){
        if(spline->state == IDLE){
            spline->points[1] = (Vector2){mouseInCanvas->x,mouseInCanvas->y};
            spline->state = MAKING_LINE;
        }
        else if(spline->state == BENDING){
            *lastMouse = *mouseInCanvas;
        }
        
    }
    else if(IsMouseButtonDown(mouse_button)){
        if(spline->state == MAKING_LINE){
            setAndDrawSplinePoints(spline,preview,mouseInCanvas,color);
        }
        else if(spline->state == BENDING){
            Vector2 previewPoints[4];
            memcpy(previewPoints, spline->points, sizeof(Vector2) * spline->max_points);    
            Vector2 bend_vector = (Vector2){(mouseInCanvas->x - lastMouse->x) * 5,(mouseInCanvas->y - lastMouse->y) * 5};
            previewPoints[spline->index] = (Vector2){previewPoints[spline->index].x - bend_vector.x,previewPoints[spline->index].y - bend_vector.y};
            BeginTextureMode(*preview);
            ClearBackground(BLANK);
            DrawSplineCatmullRom(previewPoints,spline->max_points,spline->thickness, color);
            EndTextureMode();
        }
    }    
    else if(IsMouseButtonReleased(mouse_button))
    {
        if(spline->state == MAKING_LINE){
            setAndDrawSplinePoints(spline,preview,mouseInCanvas,color);
            spline->state = BENDING; 
        }
        else if(spline->state == BENDING){
            BeginTextureMode(*preview);
            ClearBackground(BLANK);
            EndTextureMode();
            Vector2 bend_vector = (Vector2){(mouseInCanvas->x - lastMouse->x) * 5,(mouseInCanvas->y - lastMouse->y) * 5};
            spline->points[spline->index] = (Vector2){spline->points[spline->index].x - bend_vector.x,spline->points[spline->index].y - bend_vector.y};
            if(spline->index == spline->max_points -1){
                BeginTextureMode(*canvas);
                DrawSplineCatmullRom(spline->points,spline->max_points,spline->thickness, color);
                EndTextureMode();
                spline->state = IDLE; 
                spline->index = 0;
            }
            else{
                BeginTextureMode(*preview);
                DrawSplineCatmullRom(spline->points,spline->max_points,spline->thickness, color);
                EndTextureMode();
                spline->index = spline->max_points -1;
            }

        }

    }
}

// POLYGON FUNCTIONS

int Comparer(const void *a, const void *b){
    Vector2 *v1 = (Vector2 *)a;
    Vector2 *v2 = (Vector2 *)b;

    return v1->x - v2->x;
}

void fillPolygon(Polygon *poly, Color fill_color) {
    Vector2 *intersections;
    int num_of_intersections;

    for (int y = poly->minY; y <= poly->maxY; y++) {
        intersections = malloc(sizeof(Vector2) * poly->num_of_vertices);
        num_of_intersections = 0;

        for (int i = 0; i < poly->num_of_vertices; i++) {
            float x1 = poly->vertices[i].x;
            float y1 = poly->vertices[i].y;
            float x2 = poly->vertices[(i + 1) % poly->num_of_vertices].x;
            float y2 = poly->vertices[(i + 1) % poly->num_of_vertices].y;

            if (y1 == y2) continue; // skip horizontal edges

            float ymin = fmin(y1, y2);
            float ymax = fmax(y1, y2);

            if (y > ymin && y <= ymax) {
                float t = (y - y1) / (y2 - y1);
                float x = x1 + t * (x2 - x1);
                intersections[num_of_intersections++] = (Vector2){x, y};
            }
        }

        qsort(intersections, num_of_intersections, sizeof(Vector2), Comparer);

        for (int i = 0; i < num_of_intersections - 1; i += 2) {
            DrawLineEx(intersections[i], intersections[i + 1], 1, fill_color);
        }

        free(intersections);
    }
}

void drawPolygon(RenderTexture2D *canvas, RenderTexture2D *preview, Vector2 *lastMouse, Vector2 *mouseInCanvas,Color outline_color, Color fill_color,Polygon *poly){
    
    if(poly->num_of_vertices > 2){
        float dist = distanceBetweenVectors(poly->vertices[0],*mouseInCanvas);
        if(dist < poly->outline_size + 3.0f){

            BeginTextureMode(*preview);
            ClearBackground(BLANK);
            EndTextureMode();
            BeginTextureMode(*canvas);
            if(poly->is_filled){
                fillPolygon(poly,fill_color);
            }
            for(int i = 0; i < poly->num_of_vertices;i++)
            {
                if(poly->has_outline){
                    DrawLineEx(poly->vertices[i],poly->vertices[(i+1)%poly->num_of_vertices],poly->outline_size,outline_color);
                    DrawCircle(poly->vertices[(i+1)%poly->num_of_vertices].x,poly->vertices[(i+1)%poly->num_of_vertices].y,poly->outline_size/2,outline_color);
                }
                else{
                    DrawLineEx(poly->vertices[i],poly->vertices[(i+1)%poly->num_of_vertices],1,fill_color);
                }
                
            }
            EndTextureMode();
            createNewVertices(poly);
            lastMouse->x = -1;
            lastMouse->y = -1;
            return;
        }  
    }
    addVertexToPolygon(poly,*mouseInCanvas);
    if(poly->num_of_vertices > 1)
    {
        BeginTextureMode(*preview);
        if(poly->has_outline){
            DrawLineEx(poly->vertices[poly->num_of_vertices-2],poly->vertices[poly->num_of_vertices-1],poly->outline_size,outline_color);
            DrawCircle(poly->vertices[poly->num_of_vertices-1].x,poly->vertices[poly->num_of_vertices-1].y,poly->outline_size/2,outline_color);
        }
        else{
            DrawLineEx(poly->vertices[poly->num_of_vertices-2],poly->vertices[poly->num_of_vertices-1],1,fill_color);
        }
        EndTextureMode();
    }
}



// FILL FUNCTIONS

bool isInsideBounds(int width, int height, int x, int y)
{
    if(x >= 0 && x < width)
        if(y >= 0 && y < height)
            return true;
    return false;
}

int get_next_pixels(int width, int height, Image canvasImage, Vector2 *current_pixel, Color current_pixel_color,Color *colors_from_image, Vector2 *next_pixels[])
{
    int n = 0;

    Vector2 adjacent_pixels[4] = {
        {current_pixel->x - 1,current_pixel->y},
        {current_pixel->x + 1,current_pixel->y},
        {current_pixel->x,current_pixel->y - 1},
        {current_pixel->x,current_pixel->y + 1}
    };
    int color_index;
    for(int i = 0; i < 4; i++){
        
        if(isInsideBounds(width,height,adjacent_pixels[i].x,adjacent_pixels[i].y))
        {
            color_index = (height - adjacent_pixels[i].y -1 ) * width + adjacent_pixels[i].x;
            Color adjacent_pixel_color = colors_from_image[color_index];
            if(ColorIsEqual(current_pixel_color,adjacent_pixel_color))
            {
                Vector2 *copy = malloc(sizeof(Vector2));
                *copy = adjacent_pixels[i];
                next_pixels[n++] = copy;
            }
        }
    }

    return n;
}

void fill(RenderTexture2D *canvas, Vector2 first_pixel, Color new_color)
{
    int width = canvas->texture.width;
    int height = canvas->texture.height;

    if(!isInsideBounds(width,height,first_pixel.x,first_pixel.y)) return;

    Image canvasImage = LoadImageFromTexture(canvas->texture);

    Color *all_colors_from_image = LoadImageColors(canvasImage);

    int initial_color_index =(height - first_pixel.y -1 ) * width + first_pixel.x;

    Color initial_color = all_colors_from_image[initial_color_index];

    if(ColorIsEqual(initial_color,new_color)) return;

    Vector2 *open = malloc((width*height)*sizeof(Vector2));
    int top_of_open_stack = 0;
    bool *visited = calloc(width * height, sizeof(bool));

    open[top_of_open_stack++] = first_pixel;

    Vector2 *next_pixels[4];

    int n;

    while(top_of_open_stack > 0)
    {
        Vector2 currentPixel = open[--top_of_open_stack];
        int current_color_index = (height - currentPixel.y -1 ) * width + currentPixel.x;
        all_colors_from_image[current_color_index] = new_color;
        visited[(int)currentPixel.y * width + (int)currentPixel.x] = true;
        n = get_next_pixels(width,height,canvasImage,&currentPixel,initial_color,all_colors_from_image,next_pixels);
        for(int i = 0; i < n; i++)
        {
            if(!visited[(int)next_pixels[i]->y * width + (int)next_pixels[i]->x]){
                open[top_of_open_stack++] = *next_pixels[i];
                visited[(int)next_pixels[i]->y * width + (int)next_pixels[i]->x] = true;
            }
            free(next_pixels[i]);
        }
    }
    UpdateTexture(canvas->texture,all_colors_from_image);
    UnloadImageColors(all_colors_from_image);
    UnloadImage(canvasImage);
    free(open);
    free(visited);
}

// AIRBRUSH FUNCTIONS

void DrawAirbrush(RenderTexture2D *canvas, Vector2 mousePos, Color color, int radius, float sprayRate, float *dotAccumulator) {

    float deltaTime = GetFrameTime();
    *dotAccumulator += sprayRate * deltaTime;

    int dotsToDraw = (int)(*dotAccumulator);
    *dotAccumulator -= dotsToDraw;

    if (dotsToDraw <= 0) return;

    BeginTextureMode(*canvas);

    for (int i = 0; i < dotsToDraw; i++) {
        // get an angle between [0,360] and convert to radians
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        // get random number between [0,10000] normalize it so it gets a float between [0,1] and then multiply by the radius to get the distance to the center.
        float dist = sqrtf(GetRandomValue(0, 10000) / 10000.0f) * radius;
        // get dot position
        int x = (int)(mousePos.x + cosf(angle) * dist);
        int y = (int)(mousePos.y + sinf(angle) * dist);
        DrawPixel(x, y, color);
    }

    EndTextureMode();

}

// TEXT FUNCTIONS

void updateNewLineIndex(Text *text) {
    text->last_newline_index = 0; // Default: start of buffer

    for (int i = text->text_length - 1; i >= 0; i--) {
        if (text->buffer[i] == '\n') {
            text->last_newline_index = i+1;
            break;
        }
    }
}

void addCharToBuffer(Text *text, char new_char)
{
    if(text->text_length + 2 >=  text->buffer_size)
    {
        text->buffer_size *= 2;
        char *newBuffer = realloc(text->buffer,text->buffer_size);
        if(!newBuffer){
            fprintf(stderr, "Error: failed to reallocate memory to text buffer.\n");
            return;
        }
        text->buffer = newBuffer;
    }

    text->buffer[text->text_length++] = new_char;
    text->buffer[text->text_length] = '\0';
}


void UpdateText(Text *text){
    int char_key = GetCharPressed();

    while(char_key > 0){
        addCharToBuffer(text,(char)char_key);
        char_key = GetCharPressed();
    }

    int key = GetKeyPressed();
    while(key > 0){
        if(IsKeyPressed(KEY_ENTER)){
            addCharToBuffer(text,'\n');
            text->line_count++;
            text->last_newline_index = text->text_length;
        }

        if(IsKeyPressed(KEY_BACKSPACE) && text->text_length > 0){
            char erasedChar = text->buffer[text->text_length-1];
            text->buffer[--text->text_length] = '\0';
            if(erasedChar == '\n'){
                text->line_count--;
                updateNewLineIndex(text);
            }
        }
        key = GetKeyPressed();
    }
    
}


void DrawCaret(RenderTexture2D *target,Text *text, Color color)
{
    Vector2 caretPos = MeasureTextEx(GetFontDefault(),&text->buffer[text->last_newline_index], text->font_size, 2.0f);
    caretPos.x += text->pos.x;
    caretPos.y = text->pos.y + text->line_count * text->font_size;

    // Blink caret: show it every ~0.5s
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawLineV(caretPos, (Vector2){caretPos.x, caretPos.y + text->font_size}, color);
    }
}

void DrawTextToScreen(RenderTexture2D *target,Text *text, Color color){
    BeginTextureMode(*target);
    if(text->is_writing){
        ClearBackground(BLANK); // Only clears background if the target is the preview.
        DrawCaret(target,text,DARKGRAY);
    }
    DrawTextEx(GetFontDefault(),text->buffer,text->pos,text->font_size,2,color);

    EndTextureMode();
}


//GUI FUNCTIONS

void brushSettingsGUI(void *tool, Rectangle GUIRec){
    Brush *brush = (Brush *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 180,GetScreenHeight() - 60, 130, 15 }, "Size", TextFormat("%.0f", brush->size),&brush->size, 1, 120);
    const char *brushModeToggles = "CIRCLE;SQUARE"; 
    GuiComboBox((Rectangle){GetScreenWidth() - 205,GetScreenHeight() - 90, 170, 15 },brushModeToggles,&brush->brushModeIndex);
    brush->mode = (BrushMode)brush->brushModeIndex;
}

void airBrushSettingsGUI(void *tool, Rectangle GUIRec){
    AirBrush *airbrush = (AirBrush *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 180,GetScreenHeight() - 90, 120, 15 }, "Radius", TextFormat("%.2f", airbrush->radius),&airbrush->radius, 1, 120);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 180,GetScreenHeight() - 60, 120, 15 }, "Spray Rate", TextFormat("%.0f", airbrush->spray_rate),&airbrush->spray_rate, 1000,20000);
}

void textSettingsGUI(void *tool, Rectangle GUIRec)
{
    Text *text = (Text *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiSpinner((Rectangle){ GetScreenWidth() - 180,GetScreenHeight() - 60, 150, 20}, "Font Size ",&text->font_size,5,1000,false);
}

void shapeSettingsGUI(void *tool, Rectangle GUIRec){
    Shape *shape = (Shape *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiCheckBox((Rectangle){ GetScreenWidth() - 220,GetScreenHeight() - 120, 20, 20},"Outline",&shape->has_outline);
    GuiCheckBox((Rectangle){ GetScreenWidth() - 220,GetScreenHeight() - 90, 20, 20},"Fill",&shape->is_filled);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 155,GetScreenHeight() - 60, 110, 20},"Outline Size",TextFormat("%.0f", shape->outline_size),&shape->outline_size,1,120);  
}

void polygonSettingsGUI(void *tool, Rectangle GUIRec){
    Polygon *poly = (Polygon *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiCheckBox((Rectangle){ GetScreenWidth() - 220,GetScreenHeight() - 120, 20, 20},"Outline",&poly->has_outline);
    GuiCheckBox((Rectangle){ GetScreenWidth() - 220,GetScreenHeight() - 90, 20, 20},"Fill",&poly->is_filled);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 155,GetScreenHeight() - 60, 110, 20},"Outline Size",TextFormat("%.0f", poly->outline_size),&poly->outline_size,1,120);  
}

void magnifierSettingsGUI(void *tool, Rectangle GUIRec){
    float *zoom_percentage = (float *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 210,GetScreenHeight() - 60, 150, 20}, "Zoom",TextFormat("%.2fx", (*zoom_percentage)/100),zoom_percentage,10,800);
}

void lineSettingsGUI(void *tool, Rectangle GUIRec){
    float *lineSize = (float *)tool;
    DrawRectangleRec(GUIRec,MENU_GRAY);
    DrawRectangleLinesEx(GUIRec,1,GRAY);
    GuiSliderBar((Rectangle){ GetScreenWidth() - 195,GetScreenHeight() - 60, 150, 20}, "Line Size",TextFormat("%.0f", (*lineSize)),lineSize,1,120);
}


void resizeCanvas(RenderTexture2D *canvas,RenderTexture2D *preview,int widthIncrement, int heightIncrement, Color backgroundColor){
    int newWidth = canvas->texture.width + widthIncrement;
    int newHeight = canvas->texture.height + heightIncrement;
    newWidth = newWidth < 0 ? 0 : newWidth;
    newHeight = newHeight < 0 ? 0 : newHeight;
    RenderTexture2D newCanvas = LoadRenderTexture(newWidth,newHeight);
    RenderTexture2D newPreview = LoadRenderTexture(newWidth,newHeight);
    BeginTextureMode(newCanvas);
        ClearBackground(backgroundColor);
        Rectangle source = {0,0,canvas->texture.width,-canvas->texture.height};
        Vector2 position = {0,0};
        DrawTextureRec(canvas->texture,source,position,WHITE);
    EndTextureMode();
    BeginTextureMode(newPreview);
        ClearBackground(BLANK);
    EndTextureMode();
    UnloadRenderTexture(*canvas);
    *canvas = newCanvas;
    UnloadRenderTexture(*preview);
    *preview = newPreview;
}

void changeResizeSquaresPosition(Rectangle *resizeSquare, Rectangle *resizeHSquare, Rectangle *resizeVSquare, Vector2 canvasPos, int canvasWidth, int canvasHeight, float cameraZoom){
    resizeHSquare->x = resizeSquare->x = canvasWidth;
    resizeVSquare->x =  (canvasWidth/2 - RESIZE_SQUARE_SIDE_SIZE/2);
    resizeVSquare->y = resizeSquare->y = canvasHeight;
    resizeHSquare->y = (canvasHeight/2 - RESIZE_SQUARE_SIDE_SIZE/2);
}

void handleResizeSquaresZoom(Rectangle *resizeSquare, Rectangle *resizeHSquare, Rectangle *resizeVSquare,float cameraZoom){
    resizeHSquare->width = RESIZE_SQUARE_SIDE_SIZE / cameraZoom;
    resizeHSquare->height =  RESIZE_SQUARE_SIDE_SIZE / cameraZoom;
    resizeVSquare->width = RESIZE_SQUARE_SIDE_SIZE / cameraZoom;
    resizeVSquare->height =  RESIZE_SQUARE_SIDE_SIZE / cameraZoom;
    resizeSquare->width =  RESIZE_SQUARE_SIDE_SIZE / cameraZoom;
    resizeSquare->height =  RESIZE_SQUARE_SIDE_SIZE / cameraZoom;
}


//MAIN

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    int canvasWidth = 1920;
    int canvasHeight = 1080;

    InitWindow(screenWidth, screenHeight, "C-Paint");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(460,460);
    
    Color colors[MAX_COLORS_COUNT] = {
        BLACK, DARKGRAY, GRAY, MAROON, RED, ORANGE, GOLD, YELLOW, GREEN, LIME, DARKGREEN, SKYBLUE, BLUE, DARKBLUE,
        WHITE, RAYWHITE, LIGHTGRAY, MAGENTA, PINK, BEIGE, BROWN, DARKBROWN, PURPLE, VIOLET, DARKPURPLE, BLANK, BLANK, BLANK,
        BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK, BLANK               
    };

    int iconCodes[MAX_TOOLS_COUNT] = {
        ICON_PENCIL_BIG, ICON_RUBBER,ICON_EXPLOSION ,ICON_COLOR_BUCKET, ICON_COLOR_PICKER, ICON_TEXT_T, ICON_LENS,
        ICON_CROSSLINE, ICON_WAVE_SINUS, ICON_PLAYER_STOP, ICON_BREAKPOINT_OFF, ICON_BOX_CORNERS_BIG
    };

    Rectangle colorSquares[MAX_COLORS_COUNT] = {0};

    for (int i = 0; i < MAX_COLORS_COUNT; i++)
    {
        colorSquares[i].x = 100 + 20 * (i%14) + 5 * (i%14);
        colorSquares[i].y = 40 + 20 * (i/14) + 5 * (i/14);
        colorSquares[i].width = 20;
        colorSquares[i].height = 20;
    }

    Rectangle primaryColorSquare = {25,50,30,30};
    Rectangle secondaryColorSquare = {primaryColorSquare.x + primaryColorSquare.width/2, primaryColorSquare.y + primaryColorSquare.height/2,30,30};

    Rectangle toolSquares[MAX_TOOLS_COUNT] = {0};


    for (int i = 0; i < MAX_TOOLS_COUNT; i++)
    {
        toolSquares[i].x = 15 + 30 * (i%2) + 10 * (i%2);
        toolSquares[i].y = 122 + 30 * (i/2) + 10 * (i/2);
        toolSquares[i].width = toolSquares[i].height = 30;
    }

    RenderTexture2D canvas = LoadRenderTexture(canvasWidth,canvasHeight);
    RenderTexture2D preview = LoadRenderTexture(canvasWidth,canvasHeight);
    Vector2 canvasPos = {
        102,
        122
    };

    Color backgroundColor = WHITE;

    BeginTextureMode(canvas);
        ClearBackground(backgroundColor);  
    EndTextureMode();

    BeginTextureMode(preview);
        ClearBackground(BLANK);  
    EndTextureMode();

    Rectangle resizeSquare = (Rectangle){canvasWidth,canvasHeight,RESIZE_SQUARE_SIDE_SIZE,RESIZE_SQUARE_SIDE_SIZE};
    Rectangle resizeHorizontallySquare = (Rectangle){canvasWidth,canvasHeight/2-RESIZE_SQUARE_SIDE_SIZE/2,RESIZE_SQUARE_SIDE_SIZE,RESIZE_SQUARE_SIDE_SIZE};
    Rectangle resizeVerticallySquare = (Rectangle){canvasWidth/2-RESIZE_SQUARE_SIDE_SIZE/2,canvasHeight,RESIZE_SQUARE_SIDE_SIZE,RESIZE_SQUARE_SIDE_SIZE};

    int currentTool = BRUSH;

    Color primaryColor =BLACK;
    Color secondaryColor = WHITE;
    Color *changedColor = NULL;

    bool colorPickerOpen = false;

    // TOOLS
    Brush *currentBrush = brush(5,ROUND);
    Brush *currentEraser = brush(5,SQUARE);
    AirBrush *currentAirBrush = airbrush(20,3000.0f);
    Text *currentText = text(20);
    Shape *currentRec = shape(5,true,false);
    Shape *currentOval = shape(5,true,false);
    Polygon *currentPoly = polygon(1,true,true);
    Spline *currentSpline = spline(4,5);


    float lineSize = 5;

    Vector2 lastMouse = { -1, -1 };

    SetTargetFPS(0); 

    float dotAccumulator = 0.0f;

    int currentGesture = GESTURE_NONE;

    bool isMouseOverCanvas = false;

    bool resizingCanvas = false;
    bool resizingWidth = false;
    bool resizingHeight = false;

    int widthIncrement = 0;
    int heightIncrement = 0;

    Camera2D camera = {0};
    camera.zoom = 1.0f;
    camera.offset = canvasPos;
    camera.target = (Vector2){0,0};
    float zoom_percentage = camera.zoom * 100;

    int horizontalScroll = 0;
    int verticalScroll = 0;

    float visibleWidth;
    float visibleHeight;

    int horizontalScrollMax;
    int verticalScrollMax;


    void *tools[MAX_TOOLS_COUNT] = {
        [BRUSH] = currentBrush,
        [ERASER] = currentEraser,
        [AIR_BRUSH] = currentAirBrush,
        [COLOR_BUCKET] = NULL,
        [COLOR_PICKER] = NULL,
        [TEXT_BOX] = currentText,
        [MAGNIFIER] = &zoom_percentage,
        [LINE] = &lineSize,
        [CURVE] = &currentSpline->thickness,
        [RECTANGLE] = currentRec,
        [OVAL] = currentOval,
        [POLYGON] = currentPoly
    };

    GUIFunc GUISettingFunctions[MAX_TOOLS_COUNT];
    GUISettingFunctions[BRUSH] =  brushSettingsGUI;
    GUISettingFunctions[ERASER] = brushSettingsGUI;
    GUISettingFunctions[AIR_BRUSH] = airBrushSettingsGUI;
    GUISettingFunctions[COLOR_BUCKET] = NULL;
    GUISettingFunctions[COLOR_PICKER] = NULL;
    GUISettingFunctions[TEXT_BOX] = textSettingsGUI;
    GUISettingFunctions[MAGNIFIER] = magnifierSettingsGUI;
    GUISettingFunctions[LINE] = lineSettingsGUI;
    GUISettingFunctions[CURVE] = lineSettingsGUI;
    GUISettingFunctions[RECTANGLE] = shapeSettingsGUI;
    GUISettingFunctions[OVAL] = shapeSettingsGUI;
    GUISettingFunctions[POLYGON] = polygonSettingsGUI;


    void *currentToolPtr = currentBrush;

    while (!WindowShouldClose())
    {
        visibleWidth = GetScreenWidth() / camera.zoom - canvasPos.x - RESIZE_SQUARE_SIDE_SIZE*2;
        visibleHeight = GetScreenHeight() / camera.zoom - canvasPos.y - RESIZE_SQUARE_SIDE_SIZE*4;

        horizontalScrollMax = canvasWidth - (int)visibleWidth + 30;
        verticalScrollMax = canvasHeight - (int)visibleHeight + 50;

        horizontalScrollMax = (horizontalScrollMax < 0) ? 0 : horizontalScrollMax;
        verticalScrollMax = (verticalScrollMax < 0) ? 0 : verticalScrollMax;

        Rectangle menuRec = (Rectangle){0, 0, GetScreenWidth(), 120};
        Rectangle menuRec2 = (Rectangle){0,120,100,GetScreenHeight()};
        Rectangle footerRec = (Rectangle){0,GetScreenHeight() - 20,GetScreenWidth(),20};
        Rectangle HorizontalScrollBar = {canvasPos.x,GetScreenHeight() - 30,GetScreenWidth() - canvasPos.x - 10,10};
        Rectangle VerticalScrollBar = {GetScreenWidth() - 10, canvasPos.y,10,GetScreenHeight()-canvasPos.y-30};

        Rectangle GUIRecs[MAX_TOOLS_COUNT] = {
            [BRUSH] = (Rectangle){GetScreenWidth() - 220,GetScreenHeight() - 105,200,70},
            [ERASER] = (Rectangle){GetScreenWidth() - 220,GetScreenHeight() - 105,200,70},
            [AIR_BRUSH] = (Rectangle){GetScreenWidth() - 250,GetScreenHeight() - 105,235,70},
            [COLOR_BUCKET] = (Rectangle){0},
            [COLOR_PICKER] = (Rectangle){0},
            [TEXT_BOX] = (Rectangle){GetScreenWidth() - 250,GetScreenHeight() - 70,235,35},
            [MAGNIFIER] = (Rectangle){GetScreenWidth() - 250,GetScreenHeight() - 70,235,35},
            [LINE] = (Rectangle){GetScreenWidth() - 250,GetScreenHeight() - 70,235,35},
            [CURVE] = (Rectangle){GetScreenWidth() - 250,GetScreenHeight() - 70,235,35},
            [RECTANGLE] = (Rectangle){GetScreenWidth() - 230,GetScreenHeight() - 130,215,95},
            [OVAL] = (Rectangle){GetScreenWidth() - 230,GetScreenHeight() - 130,215,95},
            [POLYGON] = (Rectangle){GetScreenWidth() - 230,GetScreenHeight() - 130,215,95},
        };

        currentGesture = GetGestureDetected();     
        // printf("%d\n",currentBrush->size);
        Vector2 mouse = GetMousePosition();
        Vector2 mouseInCanvas = GetScreenToWorld2D(GetMousePosition(), camera);

        if(
            isInsideBounds(canvas.texture.width,canvas.texture.height,mouseInCanvas.x,mouseInCanvas.y) 
            && !CheckCollisionPointRec(mouse,menuRec) 
            && !CheckCollisionPointRec(mouse,menuRec2)
            && !CheckCollisionPointRec(mouse,footerRec)
            && !(visibleWidth < canvasWidth && CheckCollisionPointRec(mouse,HorizontalScrollBar))
            && !(visibleHeight < canvasHeight && CheckCollisionPointRec(mouse,VerticalScrollBar))
            && !CheckCollisionPointRec(mouse,GUIRecs[currentTool])
            && !colorPickerOpen 
            && !resizingCanvas
        )
            isMouseOverCanvas = true;
        else
            isMouseOverCanvas = false;

        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (CheckCollisionPointRec(mouse,colorSquares[i]))
            {
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && !ColorIsEqual(colors[i],BLANK))
                    primaryColor = colors[i];
                
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT) && !ColorIsEqual(colors[i],BLANK))
                    secondaryColor = colors[i];
                
                if(currentGesture == GESTURE_DOUBLETAP){
                    changedColor = &colors[i];
                    colorPickerOpen = true;
                }
            }
        }

        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){
            if(CheckCollisionPointRec(mouseInCanvas,resizeSquare)){
                resizingCanvas = true;
                resizingWidth = true;
                resizingHeight = true;

            }
            else if(CheckCollisionPointRec(mouseInCanvas,resizeHorizontallySquare)){
                resizingCanvas = true;
                resizingWidth = true;
            }
            else if(CheckCollisionPointRec(mouseInCanvas,resizeVerticallySquare)){
                resizingCanvas = true;
                resizingHeight = true;
            }
        }

        if(resizingCanvas){
            if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)){
                if(resizingWidth){
                    widthIncrement = mouseInCanvas.x - canvas.texture.width; 
                }
                if(resizingHeight){
                    heightIncrement = mouseInCanvas.y - canvas.texture.height; 
                }
            }
            if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT)){
                resizeCanvas(&canvas,&preview,widthIncrement,heightIncrement,backgroundColor);
                canvasWidth = canvas.texture.width;
                canvasHeight = canvas.texture.height;
                changeResizeSquaresPosition(&resizeSquare,&resizeHorizontallySquare,&resizeVerticallySquare,canvasPos,canvasWidth,canvasHeight,camera.zoom);
                resizingCanvas = false;
                resizingWidth = false;
                resizingHeight = false;
                widthIncrement = 0;
                heightIncrement = 0;
            }
        }

        int increment = GetMouseWheelMove();
        switch (currentTool)
        {
            case BRUSH:
                if(isMouseOverCanvas){
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                    {
                        paint(&canvas,&mouseInCanvas,&lastMouse,currentBrush,primaryColor);
                    }
                    else if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON) )
                    {  
                        paint(&canvas,&mouseInCanvas,&lastMouse,currentBrush,secondaryColor);
                    }
                    else{
                        lastMouse.x = -1;
                        lastMouse.y = -1;
                    }
                }
                if(increment != 0)
                {
                    currentBrush->size = changeSize(currentBrush->size, increment);
                }
                break;
            case ERASER:
                if(isMouseOverCanvas){
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
                    {
                        paint(&canvas,&mouseInCanvas,&lastMouse,currentEraser,backgroundColor);
                    }
                    else{
                        lastMouse.x = -1;
                        lastMouse.y = -1;
                    }
                }
                if(increment != 0)
                {
                    currentEraser->size = changeSize(currentEraser->size, increment);
                }
                break;
            case COLOR_BUCKET:
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                    fill(&canvas,mouseInCanvas,primaryColor);
                }
                else if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
                    fill(&canvas,mouseInCanvas,secondaryColor);
                }
                break;
            case COLOR_PICKER:
                if(isMouseOverCanvas){
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)){
                        Image canvasImage = LoadImageFromTexture(canvas.texture);
                        primaryColor = GetImageColor(canvasImage,mouseInCanvas.x,canvasHeight - mouseInCanvas.y - 1);
                        UnloadImage(canvasImage);
                    }
                    else if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)){
                        Image canvasImage = LoadImageFromTexture(canvas.texture);
                        secondaryColor = GetImageColor(canvasImage,mouseInCanvas.x,canvasHeight - mouseInCanvas.y - 1);
                        UnloadImage(canvasImage);
                    }
                }
                break;
            case AIR_BRUSH:
                if(isMouseOverCanvas){
                    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                        DrawAirbrush(&canvas, mouseInCanvas, primaryColor,currentAirBrush->radius,currentAirBrush->spray_rate,&dotAccumulator); // radius 20, density 100
                    }
                    if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                        DrawAirbrush(&canvas, mouseInCanvas, secondaryColor,currentAirBrush->radius,currentAirBrush->spray_rate,&dotAccumulator); // radius 20, density 100
                    }
                }
                if(increment != 0)
                {
                    currentAirBrush->radius = changeSize(currentAirBrush->radius, increment);
                }      
                break;
            case TEXT_BOX:
                if(isMouseOverCanvas){
                    if(IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                    {
                        currentText->is_writing = !currentText->is_writing;
                        if(currentText->is_writing == true){
                            currentText->pos = mouseInCanvas;
                        }
                        else{
                            DrawTextToScreen(&canvas,currentText,primaryColor);
                            createNewTextBuffer(currentText);       
                        }
                    }
                }
                if(currentText->is_writing)
                {
                    UpdateText(currentText);
                    DrawTextToScreen(&preview,currentText,primaryColor);
                }
                break;
            case MAGNIFIER:
                if(isMouseOverCanvas){
                    if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){

                        zoom_percentage += 10;
                        if (zoom_percentage > 800){
                            zoom_percentage = 800;
                        }
                    }
                    else if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)){
                        zoom_percentage -= 10;
                        if (zoom_percentage < 10){
                            zoom_percentage = 10;
                        }
                    }
                }
                if(increment != 0){
                    float scale = 5 * increment;
                    zoom_percentage = Clamp(zoom_percentage + scale, 10, 800);  
                }
                camera.zoom = zoom_percentage/100;
                handleResizeSquaresZoom(&resizeSquare,&resizeHorizontallySquare,&resizeVerticallySquare,camera.zoom);
                break;
            case LINE:
                drawLine(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,lineSize,isMouseOverCanvas);
                drawLine(MOUSE_RIGHT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,lineSize,isMouseOverCanvas);
                if(increment != 0)
                {
                    lineSize = changeSize(lineSize, increment);
                }
                break;
            case CURVE:
                drawSpline(MOUSE_BUTTON_LEFT,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,currentSpline,isMouseOverCanvas);
                drawSpline(MOUSE_BUTTON_RIGHT,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,currentSpline,isMouseOverCanvas);
                break;
            case RECTANGLE:
                drawShape(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,primaryColor,*currentRec,drawRec,isMouseOverCanvas);
                drawShape(MOUSE_RIGHT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,secondaryColor,*currentRec,drawRec,isMouseOverCanvas);
                if(increment != 0)
                {
                    currentRec->outline_size = changeSize(currentRec->outline_size, increment);
                }
                break;
            case OVAL:
                drawShape(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,primaryColor,*currentOval,drawOval,isMouseOverCanvas);
                drawShape(MOUSE_RIGHT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,secondaryColor,*currentOval,drawOval,isMouseOverCanvas);
                break;
            case POLYGON:
                if(isMouseOverCanvas){
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                        drawPolygon(&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,secondaryColor,currentPoly);
                    }
                    else if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON))
                    {
                        drawPolygon(&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,primaryColor,currentPoly);
                    }
                }
                break;
            default:
                break;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);

        BeginMode2D(camera);
            DrawTextureRec(canvas.texture, (Rectangle){0,0,canvasWidth,-canvasHeight}, (Vector2){0,0}, WHITE);
            DrawTextureRec(preview.texture, (Rectangle){0,0,canvasWidth,-canvasHeight}, (Vector2){0,0}, WHITE);
            if(resizingCanvas){
                int resizeRecWidth = canvasWidth + widthIncrement < 0 ? 0 : canvasWidth + widthIncrement;
                int resizeRecHeight = canvasHeight + heightIncrement < 0 ? 0 : canvasHeight + heightIncrement;
                DrawRectangleLines(0,0,resizeRecWidth,resizeRecHeight,DARKBLUE);
            }

            DrawRectangleRec(resizeSquare,DARKBLUE);
            DrawRectangleRec(resizeHorizontallySquare,DARKBLUE);
            DrawRectangleRec(resizeVerticallySquare,DARKBLUE);

        EndMode2D();
        
        DrawRectangleRec(menuRec, MENU_GRAY);
        DrawRectangleRec(menuRec2, MENU_GRAY);
        DrawLine(100, 120, GetScreenWidth(), 120, LIGHTGRAY);
        DrawLine(100, 120, 100, GetScreenHeight(), LIGHTGRAY);
        DrawRectangle(0,0,GetScreenWidth(),30,LIGHTGRAY);
        //DrawRectangle(0,GetScreenHeight() - 10,GetScreenWidth(),10,Fade(LIGHTGRAY, 0.3f));
            
        DrawRectangleRec(secondaryColorSquare,secondaryColor);
        DrawRectangleLinesEx(secondaryColorSquare,1,BLACK);
        DrawRectangleRec(primaryColorSquare,primaryColor);
        DrawRectangleLinesEx(primaryColorSquare,1,BLACK);
            
        for(int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            DrawRectangleRec(colorSquares[i],colors[i]);
            DrawRectangleLinesEx(colorSquares[i],1,BLACK);
        }

        for(int i = 0; i < MAX_TOOLS_COUNT; i++)
        {
            char str[10];
            sprintf(str, "#%d#",iconCodes[i]);
            if(GuiButton(toolSquares[i],str)){
                if(currentTool == POLYGON && i != POLYGON){
                    // RESETS POLYGON TO THE INITIAL STATE WHERE IT HAS NO VERTICES AND ERASES ANY EDGE THAT WAS DRAWN IN THE PREVIEW.
                    BeginTextureMode(preview);
                    ClearBackground(BLANK);
                    EndTextureMode();
                    createNewVertices(currentPoly);
                }
                currentTool = i;
                currentToolPtr = tools[i];
            } 
        }

        GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
        if (GuiLabelButton((Rectangle){10,0,50,30}, "File"));

        DrawRectangleRec(footerRec,LIGHTGRAY);
        DrawLine(0,GetScreenHeight() - 20,GetScreenWidth(),GetScreenHeight() - 20, DARKGRAY);
        GuiLabel((Rectangle){10,GetScreenHeight() - 15,10,10},TextFormat("#%d#",ICON_CURSOR_POINTER));
        if(isMouseOverCanvas)
            DrawTextEx(GetFontDefault(), TextFormat("%d, %d px",(int)mouseInCanvas.x,(int)mouseInCanvas.y),(Vector2){40,GetScreenHeight() - 15},10, 2,DARKGRAY);

        if (GUISettingFunctions[currentTool]){
            GUISettingFunctions[currentTool](currentToolPtr, GUIRecs[currentTool]);
        }

        if(colorPickerOpen){
            if(GuiWindowBox((Rectangle){GetScreenWidth()/2 - 150,GetScreenHeight()/2 - 150,300,300},"Change Color")){
                colorPickerOpen = false;
            };
            GuiColorPicker((Rectangle){GetScreenWidth()/2 - 125,GetScreenHeight()/2 - 100,225,225},NULL,changedColor);
            changedColor->a = 255;
        }

        if(visibleWidth < canvasWidth){
            horizontalScroll = GuiScrollBar(HorizontalScrollBar,horizontalScroll,0,horizontalScrollMax);
            camera.target.x = horizontalScroll;
        }
        else{
            camera.target.x = 0;
        }

        if(visibleHeight < canvasHeight){
            verticalScroll = GuiScrollBar(VerticalScrollBar,verticalScroll,0,verticalScrollMax);
            camera.target.y = verticalScroll;
        }
        else{
            camera.target.y = 0;
        }

        EndDrawing();
        printf("%d\n",GetFPS());
        

    }
    freeBrush(currentBrush);
    freeBrush(currentEraser);
    freeAirBrush(currentAirBrush);
    freeText(currentText);
    freeShape(currentRec);
    freeShape(currentOval);
    freePolygon(currentPoly);
    freeSpline(currentSpline);
    UnloadRenderTexture(canvas);
    UnloadRenderTexture(preview);
    CloseWindow();

    return 0;
}

