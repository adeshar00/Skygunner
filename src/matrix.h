
// Matrix struct

typedef struct matrix
{
	float cell[16];
} matrix;



// Function Prototypes

void matrix_identity(matrix* mat);
void matrix_translate(matrix* mat, float x, float y, float z);
void matrix_scale(matrix* mat, float x, float y, float z);
void matrix_rotatex(matrix* mat, float cos, float sin);
void matrix_rotatey(matrix* mat, float cos, float sin);
void matrix_rotatez(matrix* mat, float cos, float sin);
void matrix_multiply(matrix* result, matrix* left, matrix* right);

void matrix_translatefw(matrix* mat, float x, float y, float z);
void matrix_scalefw(matrix* mat, float x, float y, float z);
void matrix_rotatexfw(matrix* mat, float cos, float sin);
void matrix_rotateyfw(matrix* mat, float cos, float sin);
void matrix_rotatezfw(matrix* mat, float cos, float sin);


