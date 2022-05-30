
#include "visualise.h"

//D���� ������ �� �����

void print_string(float x, float y, const char* text, float r, float g, float b) 
{
	static char buffer[99999]; // ~500 chars
	int num_quads;

	num_quads = stb_easy_font_print(x, y, text, NULL, buffer, sizeof(buffer));

	glColor3f(r, g, b);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 16, buffer);
	glDrawArrays(GL_QUADS, 0, num_quads * 4);
	glDisableClientState(GL_VERTEX_ARRAY);
}

//

/* ������� �� �������� ������� ��������� � ����������*/

float xcoord(float r, float angle)
{
	float x = r * cos(angle * PI / 180);
	return x;
}

float ycoord(float r, float angle)
{
	float y = r * sin(angle * PI / 180);
	return y;
}

/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

//��������� �����

void draw(float r, float angle, float cx, float cy, std::vector<std::string> input, float bias) // ��������� ����� �����
{
	float startangle = 0;
	int size = input.size() - 1;

	glTranslated(cx, cy, 0);
	glPointSize(25);

	for (int i = 0; i < size; i++)
	{

		startangle = angle * i + bias;
		float x = xcoord(r, startangle);
		float y = ycoord(r, startangle);

		glBegin(GL_POINTS); // ��������� ������

		glColor3f(1, 1, 1);
		glVertex2d(x, y);

		glEnd();


		glPushMatrix();
		glTranslated(x, y, 0);
		glScaled(0.0055, -0.0055, 0);
		print_string(0, 0, input[i + 1].c_str(), 0, 1, 0); // ��������� ��������
		glPopMatrix();


		glBegin(GL_LINES); // ��������� �����

		glColor3f(1, 1, 1);
		glVertex2d(x, y);
		glVertex2f(0, 0); // main point

		glEnd();
	}
}

void visualise(std::vector<std::vector<std::string>> data, std::vector<std::string> Comutators) //��������� ����� �����
{
	int size = data.size();
	float angle = 360 / size;
	float startangle = 0;


	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(1920, 1080, "Net-Map", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	/* �������� � �����, ���� �� ��������� ���� */
	while (!glfwWindowShouldClose(window))
	{
		
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glBegin(GL_POINTS);

			glVertex2d(0, 0); // ����� �����

		glEnd();

		draw(C_RAD, angle, 0, 0, Comutators, 0); // ������������ �����������

		for (int i = 0; i < size; i++)
		{
			glPushMatrix();
			startangle = angle * i;
			float x = xcoord(C_RAD, startangle);
			float y = ycoord(C_RAD, startangle);
			draw(SUB_RAD, angle, x, y, data[i], 20);  // ������������ IP-������ ���������
			glPopMatrix();
		}
		startangle = 0;


		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();
}

//