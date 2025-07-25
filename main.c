#include "include/raylib.h"
#define RAYGUI_IMPLEMENTATION
#define RAYGUI_SUPPORT_ICONS
#include "include/raygui.h"
// #include "stack.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define MAX_COLORS_COUNT 42
#define MAX_TOOLS_COUNT 12

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
    ROUND,
    SQUARE
} BrushMode;

typedef enum{
    IDLE,
    MAKING_LINE,
    BENDING
} SplineState;

// STRUCTS

typedef struct S_Brush {
    int size;
    BrushMode mode;
} Brush;

typedef struct S_AirBrush{
    int radius;
    float spray_rate;
} AirBrush;

typedef struct S_Shape {
    int outline_size;
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
} Text;

typedef struct S_Polygon
{
    int maxY;
    int minY;
    int num_of_vertices;
    int capacity;
    Vector2 *vertices;
    int outline_size;
    bool has_outline;
    bool is_filled;

} Polygon;

typedef struct S_Spline{
    int max_points;
    Vector2 *points;
    int thickness;
    SplineState state;
    int index;
} Spline;

typedef void (*drawFunc)(Vector2*,Vector2*,Color,Color,Shape);

// CONSTRUCTORS

Brush *brush(int size, BrushMode mode)
{
    Brush *brush;
    brush = malloc(sizeof(Brush));
    brush->size = size;
    brush->mode = mode;
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

    if(recInfo.has_outline)
        DrawRectangleLinesEx(rec,recInfo.outline_size,outline_color);
    if(recInfo.is_filled)
        DrawRectangleRec(rec,fill_color);
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

void drawShape(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview, Vector2 *lastMouse, Vector2 *mouseInCanvas,Color fill_color, Color outline_color,Shape shapeInfo, drawFunc draw_func){
    if(IsMouseButtonPressed(mouse_button))
    {
        *lastMouse = *mouseInCanvas;
    }
    else if(IsMouseButtonDown(mouse_button))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        if(draw_func != NULL)
        {
            draw_func(lastMouse,mouseInCanvas,fill_color,outline_color,shapeInfo);
        }
        EndTextureMode();
    }
    else if(IsMouseButtonReleased(mouse_button))
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

void drawLine(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview, Vector2 *lastMouse, Vector2 *mouseInCanvas,Color color,int lineSize){
    if(IsMouseButtonPressed(mouse_button))
    {
        *lastMouse = *mouseInCanvas;
    }
    else if(IsMouseButtonDown(mouse_button))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        DrawLineEx(*lastMouse,*mouseInCanvas,lineSize,color);
        EndTextureMode();
    }
    else if(IsMouseButtonReleased(mouse_button))
    {
        BeginTextureMode(*preview);
        ClearBackground(BLANK);
        EndTextureMode();
        BeginTextureMode(*canvas);
        DrawLineEx(*lastMouse,*mouseInCanvas,lineSize,color);
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

void drawSpline(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview,Vector2 *lastMouse,Vector2 *mouseInCanvas,Color color,Spline *spline){
    if(IsMouseButtonPressed(mouse_button)){
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
            DrawLineEx(intersections[i], intersections[i + 1], poly->outline_size, fill_color);
        }

        free(intersections);
    }
}

void drawPolygon(MouseButton mouse_button,RenderTexture2D *canvas, RenderTexture2D *preview, Vector2 *lastMouse, Vector2 *mouseInCanvas,Color outline_color, Color fill_color,Polygon *poly){
    
    if(poly->num_of_vertices > 0){
        float dist = distanceBetweenVectors(poly->vertices[0],*mouseInCanvas);
        if(poly->num_of_vertices > 2 && dist < poly->outline_size + 3.0f){

            BeginTextureMode(*preview);
            ClearBackground(BLANK);
            EndTextureMode();
            BeginTextureMode(*canvas);
            if(poly->is_filled){
                fillPolygon(poly,fill_color);
            }
            for(int i = 0; i < poly->num_of_vertices;i++)
            {
                DrawLineEx(poly->vertices[i],poly->vertices[(i+1)%poly->num_of_vertices],poly->outline_size,outline_color);
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
        DrawLineEx(poly->vertices[poly->num_of_vertices-2],poly->vertices[poly->num_of_vertices-1],poly->outline_size,outline_color);
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
        if(IsKeyPressed(KEY_ENTER))
            addCharToBuffer(text,'\n');
        if(IsKeyPressed(KEY_BACKSPACE) && text->text_length > 0){
            text->buffer[--text->text_length] = '\0';
        }
        key = GetKeyPressed();
    }
    
}


void DrawCaret(RenderTexture2D *target,Text *text, Color color)
{
    Vector2 caretPos = MeasureTextEx(GetFontDefault(), text->buffer, text->font_size, 2);
    caretPos.x += text->pos.x;
    caretPos.y = text->pos.y;

    // Blink caret: show it every ~0.5s
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawLineV(caretPos, (Vector2){caretPos.x, caretPos.y + text->font_size}, color);
        //DrawText("_", caretPos.x+1, caretPos.y, text->font_size, GRAY);
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



//MAIN

int main(void)
{
    const int screenWidth = 800;
    const int screenHeight = 600;
    
    const int canvasWidth = 1920;
    const int canvasHeight = 1080;

    InitWindow(screenWidth, screenHeight, "C-Paint");
    SetWindowState(FLAG_WINDOW_RESIZABLE);
    SetWindowMinSize(110 + 20 * 14 + 5 * 14,130);
    
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


    int currentTool = BRUSH;

    Color primaryColor =BLACK;
    Color secondaryColor = WHITE;

    // TOOLS
    Brush *currentBrush = brush(5,ROUND);
    Brush *currentEraser = brush(5,SQUARE);
    AirBrush *currentAirBrush = airbrush(20,3000.0f);
    Text *currentText = text(20);
    Shape *currentRec = shape(5,true,false);
    Shape *currentOval = shape(5,true,false);
    Polygon *currentPoly = polygon(1,true,true);
    Spline *currentSpline = spline(4,5);


    int lineSize = 5;

    Vector2 lastMouse = { -1, -1 };

    SetTargetFPS(0); 

    float dotAccumulator = 0.0f;

    while (!WindowShouldClose())
    {
        
        // printf("%d\n",currentBrush->size);
        Vector2 mouse = GetMousePosition();
        Vector2 mouseInCanvas = {
            mouse.x - canvasPos.x,
            mouse.y - canvasPos.y
        };

        for (int i = 0; i < MAX_COLORS_COUNT; i++)
        {
            if (CheckCollisionPointRec(mouse,colorSquares[i]))
            {
                if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                    primaryColor = colors[i];
                
                if(IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))
                    secondaryColor = colors[i];
            }
        }

        int increment = GetMouseWheelMove();

        switch (currentTool)
        {
            case BRUSH:
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON))
                {
                    paint(&canvas,&mouseInCanvas,&lastMouse,currentBrush,primaryColor);
                }
                else if(IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
                {  
                    paint(&canvas,&mouseInCanvas,&lastMouse,currentBrush,secondaryColor);
                }
                else{
                    lastMouse.x = -1;
                    lastMouse.y = -1;
                }

                if(increment != 0)
                {
                    currentBrush->size = changeSize(currentBrush->size, increment);
                }
                break;
            case ERASER:
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON))
                {
                    paint(&canvas,&mouseInCanvas,&lastMouse,currentEraser,backgroundColor);
                }
                else{
                    lastMouse.x = -1;
                    lastMouse.y = -1;
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
                break;
            case AIR_BRUSH:
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    DrawAirbrush(&canvas, mouseInCanvas, primaryColor,currentAirBrush->radius,currentAirBrush->spray_rate,&dotAccumulator); // radius 20, density 100
                }
                if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                    DrawAirbrush(&canvas, mouseInCanvas, secondaryColor,currentAirBrush->radius,currentAirBrush->spray_rate,&dotAccumulator); // radius 20, density 100
                }
                if(increment != 0)
                {
                    currentAirBrush->radius = changeSize(currentAirBrush->radius, increment);
                }      
                break;
            case TEXT_BOX:
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
                if(currentText->is_writing)
                {
                    UpdateText(currentText);
                    DrawTextToScreen(&preview,currentText,primaryColor);
                }
                break;
            case MAGNIFIER:
                break;
            case LINE:
                drawLine(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,lineSize);
                drawLine(MOUSE_RIGHT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,lineSize);
                if(increment != 0)
                {
                    lineSize = changeSize(lineSize, increment);
                }
                break;
            case CURVE:
                drawSpline(MOUSE_BUTTON_LEFT,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,currentSpline);
                drawSpline(MOUSE_BUTTON_RIGHT,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,currentSpline);
                break;
            case RECTANGLE:
                drawShape(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,primaryColor,*currentRec,drawRec);
                drawShape(MOUSE_RIGHT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,secondaryColor,*currentRec,drawRec);
                if(increment != 0)
                {
                    currentRec->outline_size = changeSize(currentRec->outline_size, increment);
                }
                break;
            case OVAL:
                drawShape(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,secondaryColor,primaryColor,*currentOval,drawOval);
                drawShape(MOUSE_RIGHT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,secondaryColor,*currentOval,drawOval);
                break;
            case POLYGON:
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    drawPolygon(MOUSE_LEFT_BUTTON,&canvas,&preview,&lastMouse,&mouseInCanvas,primaryColor,secondaryColor,currentPoly);
                }
                break;
            default:
                break;
        }

        BeginDrawing();

        ClearBackground(RAYWHITE);
            
        DrawRectangleLinesEx(
            (Rectangle){ canvasPos.x, canvasPos.y, canvasWidth, canvasHeight },
            1,
            DARKGRAY
        );

        DrawRectangle(0, 30, GetScreenWidth(), 90, Fade(LIGHTGRAY, 0.3f));
        DrawRectangle(0,120,100,GetScreenHeight(), Fade(LIGHTGRAY, 0.3f));
        DrawLine(100, 120, GetScreenWidth(), 120, Fade(LIGHTGRAY, 0.8f));
        DrawLine(100, 120, 100, GetScreenHeight(), Fade(LIGHTGRAY, 0.8f));
        DrawRectangle(0,0,GetScreenWidth(),30,Fade(LIGHTGRAY,0.8f));
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
            if(GuiButton(toolSquares[i],str)) currentTool = i;
        }

        GuiSetStyle(DEFAULT, TEXT_SIZE, 10);
        if (GuiLabelButton((Rectangle){10,0,50,30}, "File"));
        // if (GuiButton((Rectangle){0, 0, 160, 30}, "Red Brush")) primaryColor = RED;

        DrawTextureRec(canvas.texture, (Rectangle){0,0,canvasWidth,-canvasHeight}, canvasPos, WHITE);
        DrawTextureRec(preview.texture, (Rectangle){0,0,canvasWidth,-canvasHeight}, canvasPos, WHITE);

        DrawRectangle(0,GetScreenHeight() - 20,GetScreenWidth(),20,LIGHTGRAY);
        DrawLine(0,GetScreenHeight() - 20,GetScreenWidth(),GetScreenHeight() - 20, DARKGRAY);
        GuiLabel((Rectangle){10,GetScreenHeight() - 15,10,10},TextFormat("#%d#",ICON_CURSOR_POINTER));
        DrawTextEx(GetFontDefault(), TextFormat("%d, %d px",(int)mouseInCanvas.x,(int)mouseInCanvas.y),(Vector2){40,GetScreenHeight() - 15},10, 2,DARKGRAY);

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
    CloseWindow();

    return 0;
}

