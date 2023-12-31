#include "Demo.h"


Demo::Demo() {
}

Demo::~Demo() {
}

void Demo::Init() {

	// build and compile our shader program

	BuildShaders();

	BuildDepthMap();

	BuildColoredDinding();

	BuildColoredPlane();

	BuildKakiKursi();

	BuildKakiMeja();

	BuildMeja();

	BuildAlasKursi();

	BuildSandaranKursi();

	BuildPapanTulis();

	InitCamera();
}

void Demo::DeInit() {
	// optional: de-allocate all resources once they've outlived their purpose:
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteVertexArrays(1, &VAO2);
	glDeleteBuffers(1, &VBO2);
	glDeleteBuffers(1, &EBO2);
	glDeleteBuffers(1, &depthMapFBO);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void Demo::ProcessInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}

	// zoom camera
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS) {
		if (fovy < 90) {
			fovy += 0.0001f;
		}
	}

	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
		if (fovy > 0) {
			fovy -= 0.0001f;
		}
	}

	// update camera movement 
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		MoveCamera(CAMERA_SPEED);
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		MoveCamera(-CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		StrafeCamera(-CAMERA_SPEED);
	}

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		StrafeCamera(CAMERA_SPEED);
	}

	// update camera rotation
	double mouseX, mouseY;
	double midX = screenWidth / 2;
	double midY = screenHeight / 2;
	float angleY = 0.0f;
	float angleZ = 0.0f;

	// Get mouse position
	glfwGetCursorPos(window, &mouseX, &mouseY);
	if ((mouseX == midX) && (mouseY == midY)) {
		return;
	}

	// Set mouse position
	glfwSetCursorPos(window, midX, midY);

	// Get the direction from the mouse cursor, set a resonable maneuvering speed
	angleY = (float)((midX - mouseX)) / 1000;
	angleZ = (float)((midY - mouseY)) / 1000;

	// The higher the value is the faster the camera looks around.
	viewCamY += angleZ * 2;

	// limit the rotation around the x-axis
	if ((viewCamY - posCamY) > 8) {
		viewCamY = posCamY + 8;
	}
	if ((viewCamY - posCamY) < -8) {
		viewCamY = posCamY - 8;
	}
	RotateCamera(-angleY);
}

void Demo::Update(double deltaTime) {
	angle += (float) ((deltaTime * 1.5f) / 1000);
}

void Demo::Render() {

	glEnable(GL_DEPTH_TEST);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Step 1 Render depth of scene to texture
	glm::mat4 lightProjection, lightView;
	glm::mat4 lightSpaceMatrix;
	float near_plane = 3.0f, far_plane = 10.5f;
	lightProjection = glm::ortho(-20.0f, 10.0f, -20.0f, 10.0f, near_plane, far_plane);
	lightView = glm::lookAt(glm::vec3(-2.0f, 10.0f, -1.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0, 1.0, 0.0));
	lightSpaceMatrix = lightProjection * lightView;
	// render scene from light's point of view
	UseShader(this->depthmapShader);
	glUniformMatrix4fv(glGetUniformLocation(this->depthmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glViewport(0, 0, this->SHADOW_WIDTH, this->SHADOW_HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// Add Shadow
	DrawMeja(
		// Translate             Scale
		-10.0f, 0.0f, -9.0f,     1.9, 0.05, 1.2,

		this->depthmapShader
	);

	DrawMeja(
		// Translate             Scale
		-10.0f, 0.1f, -9.0f,     2.0, 0.05, 1.35,

		this->depthmapShader
	);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Step 2 Render scene normally using generated depth map
	glViewport(0, 0, this->screenWidth, this->screenHeight);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// Pass perspective projection matrix
	UseShader(this->shadowmapShader);
	glm::mat4 projection = glm::perspective(fovy, (GLfloat)this->screenWidth / (GLfloat)this->screenHeight, 0.1f, 100.0f);
	GLint projLoc = glGetUniformLocation(this->shadowmapShader, "projection");
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	// LookAt camera (position, target/direction, up)
	glm::mat4 view = glm::lookAt(glm::vec3(posCamX, posCamY, posCamZ), glm::vec3(viewCamX, viewCamY, viewCamZ), glm::vec3(upCamX, upCamY, upCamZ));
	GLint viewLoc = glGetUniformLocation(this->shadowmapShader, "view");
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

	// Setting Light Attributes
	glUniformMatrix4fv(glGetUniformLocation(this->shadowmapShader, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "viewPos"), posCamX, posCamY, posCamZ);
	glUniform3f(glGetUniformLocation(this->shadowmapShader, "lightPos"), -2.0f, 4.0f, -1.0f);

	// Configure Shaders
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "diffuseTexture"), 0);
	glUniform1i(glGetUniformLocation(this->shadowmapShader, "shadowMap"), 1);

	DrawColoredPlane(this->shadowmapShader);

	DrawColoredDinding(
		// Translate            Scale
		0.0f, 5.0f, -15.5f,     15, 6.5, 0.5,

		this->shadowmapShader
	);

	DrawColoredDinding(
		// Translate            Scale
		0.0f, 5.0f, 25.5f,      15, 6.5, 0.5,

		this->shadowmapShader
	);

	DrawColoredDinding(
		// Translate            Scale
		-15.5f, 5.0f, 5.0f,     0.5, 6.5, 20,

		this->shadowmapShader
	);

	DrawColoredDinding(
		// Translate            Scale
		15.5f, 5.0f, 5.0f,      0.5, 6.5, 20,

		this->shadowmapShader
	);


	DrawMeja(
		// Translate            Scale
		-10.0f, 0.0f, -9.0f,    1.9, 0.05, 1.2,

		this->shadowmapShader
	);
	DrawMeja(
		// Translate            Scale
		-10.0f, 0.1f, -9.0f,    2.0, 0.05, 1.35,
		
		this->shadowmapShader
	);

	DrawKakiMeja(
		// Translate            Scale
		-11.7f, -0.75f, -10.0f, 0.15, 0.75, 0.15,

		this->shadowmapShader
	);

	DrawKakiMeja(
		// Translate            Scale
		-11.7f, -0.75f, -8.0f,  0.15, 0.75, 0.15,

		this->shadowmapShader
	);

	DrawKakiMeja( 
		// Translate            Scale
		-8.3f, -0.75f, -10.0f,  0.15, 0.75, 0.15,

		this->shadowmapShader
	);

	DrawKakiMeja(
		// Translate            Scale
		-8.3f, -0.75f, -8.0f,   0.15, 0.75, 0.15,

		this->shadowmapShader
	);

	DrawKakiKursi(
		// Translate            Scale
		-10.5, -0.9f, -10.5f,   0.1, 0.55, 0.1,

		this->shadowmapShader
	);

	DrawKakiKursi(
		// Translate            Scale
		-9.5, -0.9f, -10.5f,    0.1, 0.55, 0.1,

		this->shadowmapShader
	);

	DrawKakiKursi(
		// Translate            Scale
		-10.5, -0.9f, -11.5f,   0.1, 0.55, 0.1,

		this->shadowmapShader
	);

	DrawKakiKursi(
		// Translate            Scale
		-9.5, -0.9f, -11.5f,    0.1, 0.55, 0.1,

		this->shadowmapShader
	);

	DrawAlasKursi(
		// Translate            Scale
		-10.0f, -0.3f, -11.0f,  0.7, 0.025, 0.7,

		this->shadowmapShader
	);

	DrawSandaranKursi(
		// Translate            Scale
		-10.0f, 2.7f, -11.1f,   0.7, 0.7, 0.025, -0.2f,

		this->shadowmapShader
	);

	DrawPapanTulis(
		// Translate          Scale
		0.0f, 4.0f, -14.5f, 3.5, 2, 0.25,

		this->shadowmapShader
	);

	// Draw Kaki Meja
	float zKaki = 1.0f;
	int separator = 0;
	while (zKaki <= 24.0f) {
		for (int xKaki = 9; xKaki > -11; xKaki -= 2) {
			DrawKakiMeja(
				// Translate              Scale
				xKaki, -0.75f, zKaki,     0.15, 0.75, 0.15,

				this->shadowmapShader
			);
		}
		zKaki += 2.0f;
		separator++;
		if (separator % 2 == 0 && separator != 0) {
			zKaki += 2.0f;
		}
	}

	// Draw Kaki Kursi
	zKaki = 3.5f;
	int separatorX = 0;
	int separatorZ = 0;
	while (zKaki <= 26.0f) {
		for (float xKaki = 8.5f; xKaki > -11; xKaki -= 1) {
			DrawKakiKursi(
				// Translate              Scale
				xKaki, -0.9f, zKaki,      0.1, 0.55, 0.1,

				this->shadowmapShader
			);
			separatorX++;
			if (separatorX % 2 == 0 && separatorX != 0) {
				xKaki -= 2;
			}
		}
		zKaki += 1.0f;
		separatorZ++;
		if (separatorZ % 2 == 0 && separatorZ != 0) {
			zKaki += 4.0f;
		}
	}

	// Draw Alas Meja
	float zMeja = 2.0f;
	while (zMeja <= 24.0f) {
		for (int xMeja = 8; xMeja > -11; xMeja -= 4) {
			DrawMeja(
				// Translate             Scale
				xMeja, 0.0f, zMeja,      1.2, 0.05, 1.2,

				this->shadowmapShader
			);
			DrawMeja(
				// Translate             Scale
				xMeja, 0.1f, zMeja,      1.35, 0.05, 1.35,

				this->shadowmapShader
			);
		}
		zMeja += 6.0f;
	}

	// Draw Alas Kursi
	float zKursi = 4.0f;
	float tambahanKursi = 0.0f;
	float kuranganKursi = 0.0f;
	while (zKursi <= 24.0f) {
		for (float xKursi = 8; xKursi > -11; xKursi -= 4) {
			DrawAlasKursi(
				// Translate             Scale
				xKursi, -0.3f, zKursi,   0.7, 0.025, 0.7,

				this->shadowmapShader
			);

			DrawSandaranKursi(
				// Translate                                                   Scale
				xKursi, 1.3f + tambahanKursi, zKursi + 0.2f - kuranganKursi,   0.7, 0.7, 0.025, 0.2f,

				this->shadowmapShader
			);
		}
		tambahanKursi += 1.2f;
		kuranganKursi += 0.1f;
		zKursi += 6.0f;
	}

	glDisable(GL_DEPTH_TEST);
}

void Demo::BuildColoredDinding() 
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &textureDinding);
	glBindTexture(GL_TEXTURE_2D, textureDinding);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("dinding.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords
		// front
		-0.5, -0.5, 0.5, 0, 0, 0.0f,  0.0f,  1.0f,// 0
		 0.5, -0.5, 0.5, 1, 0,  0.0f,  0.0f,  1.0f,// 1
		 0.5,  0.5, 0.5, 1, 1,  0.0f,  0.0f,  1.0f,// 2
		-0.5,  0.5, 0.5, 0, 1, 0.0f,  0.0f,  1.0f,// 3

		// right
		0.5, -0.5,  0.5, 0, 0, 1.0f,  0.0f,  0.0f,// 4
		0.5, -0.5, -0.5, 1, 0, 1.0f,  0.0f,  0.0f,// 5
		0.5,  0.5, -0.5, 1, 1, 1.0f,  0.0f,  0.0f,// 6
		0.5,  0.5,  0.5, 0, 1, 1.0f,  0.0f,  0.0f,// 7

		// back
		-0.5, -0.5, -0.5, 0, 0, 0.0f,  0.0f,  -1.0f, // 8 
		 0.5, -0.5, -0.5, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		 0.5,  0.5, -0.5, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-0.5,  0.5, -0.5, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-0.5, -0.5, -0.5, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-0.5, -0.5,  0.5, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-0.5,  0.5,  0.5, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-0.5,  0.5, -0.5, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		 0.5, 0.5,  0.5, 0, 0,   0.0f,  1.0f,  0.0f,  // 16
		-0.5, 0.5,  0.5, 1, 0,  0.0f,  1.0f,  0.0f,  // 17
		-0.5, 0.5, -0.5, 1, 1,  0.0f,  1.0f,  0.0f,  // 18
		 0.5, 0.5, -0.5, 0, 1,   0.0f,  1.0f,  0.0f,  // 19

		 // bottom
		 -0.5, -0.5, -0.5, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		  0.5, -0.5, -0.5, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		  0.5, -0.5,  0.5, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		 -0.5, -0.5,  0.5, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawColoredDinding(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureDinding);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildKakiKursi()
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &textureKakiKursi);
	glBindTexture(GL_TEXTURE_2D, textureKakiKursi);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("kaki.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-1.0, -1.0, 1.0, 0, 0, 0.0f,  0.0f,  1.0f, // 0
		1.0, -1.0, 1.0, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		1.0,  1.0, 1.0, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-1.0,  1.0, 1.0, 0, 1, 0.0f,  0.0f,  1.0f, // 3

		// right
		1.0,  1.0,  1.0, 0, 0, 1.0f,  0.0f,  0.0f, // 4
		1.0,  1.0, -1.0, 1, 0, 1.0f,  0.0f,  0.0f, // 5
		1.0, -1.0, -1.0, 1, 1, 1.0f,  0.0f,  0.0f, // 6
		1.0, -1.0,  1.0, 0, 1, 1.0f,  0.0f,  0.0f, // 7

		// back
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  0.0f,  -1.0f, // 8 
		1.0,  -1.0, -1.0, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		1.0,   1.0, -1.0, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-1.0,  1.0, -1.0, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-1.0, -1.0, -1.0, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-1.0, -1.0,  1.0, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-1.0,  1.0,  1.0, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-1.0,  1.0, -1.0, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		1.0, 1.0,  1.0, 0, 0,   0.0f,  1.0f,  0.0f, // 16
		-1.0, 1.0, 1.0, 1, 0,   0.0f,  1.0f,  0.0f, // 17
		-1.0, 1.0, -1.0, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		1.0, 1.0, -1.0, 0, 1,   0.0f,  1.0f,  0.0f, // 19

		// bottom
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		1.0, -1.0, -1.0, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		1.0, -1.0,  1.0, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-1.0, -1.0,  1.0, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Demo::DrawKakiKursi(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureKakiKursi);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildKakiMeja()
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &textureKakiMeja);
	glBindTexture(GL_TEXTURE_2D, textureKakiMeja);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("kaki.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-1.0, -1.0, 1.0, 0, 0, 0.0f,  0.0f,  1.0f, // 0
		1.0, -1.0, 1.0, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		1.0,  1.0, 1.0, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-1.0,  1.0, 1.0, 0, 1, 0.0f,  0.0f,  1.0f, // 3

		// right
		1.0,  1.0,  1.0, 0, 0, 1.0f,  0.0f,  0.0f, // 4
		1.0,  1.0, -1.0, 1, 0, 1.0f,  0.0f,  0.0f, // 5
		1.0, -1.0, -1.0, 1, 1, 1.0f,  0.0f,  0.0f, // 6
		1.0, -1.0,  1.0, 0, 1, 1.0f,  0.0f,  0.0f, // 7

		// back
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  0.0f,  -1.0f, // 8 
		1.0,  -1.0, -1.0, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		1.0,   1.0, -1.0, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-1.0,  1.0, -1.0, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-1.0, -1.0, -1.0, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-1.0, -1.0,  1.0, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-1.0,  1.0,  1.0, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-1.0,  1.0, -1.0, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		1.0, 1.0,  1.0, 0, 0,   0.0f,  1.0f,  0.0f, // 16
		-1.0, 1.0, 1.0, 1, 0,   0.0f,  1.0f,  0.0f, // 17
		-1.0, 1.0, -1.0, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		1.0, 1.0, -1.0, 0, 1,   0.0f,  1.0f,  0.0f, // 19

		// bottom
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		1.0, -1.0, -1.0, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		1.0, -1.0,  1.0, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-1.0, -1.0,  1.0, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawKakiMeja(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureKakiMeja);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildMeja()
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &textureMeja);
	glBindTexture(GL_TEXTURE_2D, textureMeja);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("atasmeja.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-0.5, -0.5,  0.5, 0, 0,  0.0f,  0.0f,  1.0f, // 0
		 0.5, -0.5,  0.5, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		 0.5,  0.5,  0.5, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-0.5,  0.5,  0.5, 0, 1,  0.0f,  0.0f,  1.0f, // 3
								 
		// right				 
		 0.5,  0.5,  0.5, 0, 0,  1.0f,  0.0f,  0.0f, // 4
		 0.5,  0.5, -0.5, 1, 0,  1.0f,  0.0f,  0.0f, // 5
		 0.5, -0.5, -0.5, 1, 1,  1.0f,  0.0f,  0.0f, // 6
		 0.5, -0.5,  0.5, 0, 1,  1.0f,  0.0f,  0.0f, // 7
								 
		// back					 
		-0.5, -0.5, -0.5, 0, 0,  0.0f,  0.0f, -1.0f, // 8 
		 0.5, -0.5, -0.5, 1, 0,  0.0f,  0.0f, -1.0f, // 9
		 0.5,  0.5, -0.5, 1, 1,  0.0f,  0.0f, -1.0f, // 10
		-0.5,  0.5, -0.5, 0, 1,  0.0f,  0.0f, -1.0f, // 11

		// left
		-0.5, -0.5, -0.5, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-0.5, -0.5,  0.5, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-0.5,  0.5,  0.5, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-0.5,  0.5, -0.5, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		 0.5,  0.5,  0.5, 0, 0,  0.0f,  1.0f,  0.0f, // 16
		-0.5,  0.5,  0.5, 1, 0,  0.0f,  1.0f,  0.0f, // 17
		-0.5,  0.5, -0.5, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		 0.5,  0.5, -0.5, 0, 1,  0.0f,  1.0f,  0.0f, // 19

		// bottom
		-0.5, -0.5, -0.5, 0, 0,  0.0f,  -1.0f,  0.0f, // 20
		 0.5, -0.5, -0.5, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		 0.5, -0.5,  0.5, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-0.5, -0.5,  0.5, 0, 1,  0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawMeja(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureMeja);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildAlasKursi()
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &textureAlasKursi);
	glBindTexture(GL_TEXTURE_2D, textureAlasKursi);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("kaki.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-1.0, -1.0, 1.0, 0, 0, 0.0f,  0.0f,  1.0f, // 0
		1.0, -1.0, 1.0, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		1.0,  1.0, 1.0, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-1.0,  1.0, 1.0, 0, 1, 0.0f,  0.0f,  1.0f, // 3

		// right
		1.0,  1.0,  1.0, 0, 0, 1.0f,  0.0f,  0.0f, // 4
		1.0,  1.0, -1.0, 1, 0, 1.0f,  0.0f,  0.0f, // 5
		1.0, -1.0, -1.0, 1, 1, 1.0f,  0.0f,  0.0f, // 6
		1.0, -1.0,  1.0, 0, 1, 1.0f,  0.0f,  0.0f, // 7

		// back
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  0.0f,  -1.0f, // 8 
		1.0,  -1.0, -1.0, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		1.0,   1.0, -1.0, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-1.0,  1.0, -1.0, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-1.0, -1.0, -1.0, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-1.0, -1.0,  1.0, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-1.0,  1.0,  1.0, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-1.0,  1.0, -1.0, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		1.0, 1.0,  1.0, 0, 0,   0.0f,  1.0f,  0.0f, // 16
		-1.0, 1.0, 1.0, 1, 0,   0.0f,  1.0f,  0.0f, // 17
		-1.0, 1.0, -1.0, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		1.0, 1.0, -1.0, 0, 1,   0.0f,  1.0f,  0.0f, // 19

		// bottom
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		1.0, -1.0, -1.0, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		1.0, -1.0,  1.0, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-1.0, -1.0,  1.0, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawAlasKursi(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureAlasKursi);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildSandaranKursi()
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &textureSandaranKursi);
	glBindTexture(GL_TEXTURE_2D, textureSandaranKursi);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("sandarankursi.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-1.0, -1.0, 1.0, 0, 0, 0.0f,  0.0f,  1.0f, // 0
		1.0, -1.0, 1.0, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		1.0,  1.0, 1.0, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-1.0,  1.0, 1.0, 0, 1, 0.0f,  0.0f,  1.0f, // 3

		// right
		1.0,  1.0,  1.0, 0, 0, 1.0f,  0.0f,  0.0f, // 4
		1.0,  1.0, -1.0, 1, 0, 1.0f,  0.0f,  0.0f, // 5
		1.0, -1.0, -1.0, 1, 1, 1.0f,  0.0f,  0.0f, // 6
		1.0, -1.0,  1.0, 0, 1, 1.0f,  0.0f,  0.0f, // 7

		// back
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  0.0f,  -1.0f, // 8 
		1.0,  -1.0, -1.0, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		1.0,   1.0, -1.0, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-1.0,  1.0, -1.0, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-1.0, -1.0, -1.0, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-1.0, -1.0,  1.0, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-1.0,  1.0,  1.0, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-1.0,  1.0, -1.0, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		1.0, 1.0,  1.0, 0, 0,   0.0f,  1.0f,  0.0f, // 16
		-1.0, 1.0, 1.0, 1, 0,   0.0f,  1.0f,  0.0f, // 17
		-1.0, 1.0, -1.0, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		1.0, 1.0, -1.0, 0, 1,   0.0f,  1.0f,  0.0f, // 19

		// bottom
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		1.0, -1.0, -1.0, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		1.0, -1.0,  1.0, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-1.0, -1.0,  1.0, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawSandaranKursi(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, float rotate, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textureSandaranKursi);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::rotate(model, rotate, glm::vec3(1, 0, 0));

	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildPapanTulis()
{
	// load image into texture memory
	// Load and create a texture 
	glGenTextures(1, &texturePapanTulis);
	glBindTexture(GL_TEXTURE_2D, texturePapanTulis);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	int width, height;
	unsigned char* image = SOIL_load_image("papantulis.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	float vertices[] = {
		// format position, tex coords, normal
		// front
		-1.0, -1.0, 1.0, 0, 0, 0.0f,  0.0f,  1.0f, // 0
		1.0, -1.0, 1.0, 1, 0,  0.0f,  0.0f,  1.0f, // 1
		1.0,  1.0, 1.0, 1, 1,  0.0f,  0.0f,  1.0f, // 2
		-1.0,  1.0, 1.0, 0, 1, 0.0f,  0.0f,  1.0f, // 3

		// right
		1.0,  1.0,  1.0, 0, 0, 1.0f,  0.0f,  0.0f, // 4
		1.0,  1.0, -1.0, 1, 0, 1.0f,  0.0f,  0.0f, // 5
		1.0, -1.0, -1.0, 1, 1, 1.0f,  0.0f,  0.0f, // 6
		1.0, -1.0,  1.0, 0, 1, 1.0f,  0.0f,  0.0f, // 7

		// back
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  0.0f,  -1.0f, // 8 
		1.0,  -1.0, -1.0, 1, 0, 0.0f,  0.0f,  -1.0f, // 9
		1.0,   1.0, -1.0, 1, 1, 0.0f,  0.0f,  -1.0f, // 10
		-1.0,  1.0, -1.0, 0, 1, 0.0f,  0.0f,  -1.0f, // 11

		// left
		-1.0, -1.0, -1.0, 0, 0, -1.0f,  0.0f,  0.0f, // 12
		-1.0, -1.0,  1.0, 1, 0, -1.0f,  0.0f,  0.0f, // 13
		-1.0,  1.0,  1.0, 1, 1, -1.0f,  0.0f,  0.0f, // 14
		-1.0,  1.0, -1.0, 0, 1, -1.0f,  0.0f,  0.0f, // 15

		// upper
		1.0, 1.0,  1.0, 0, 0,   0.0f,  1.0f,  0.0f, // 16
		-1.0, 1.0, 1.0, 1, 0,   0.0f,  1.0f,  0.0f, // 17
		-1.0, 1.0, -1.0, 1, 1,  0.0f,  1.0f,  0.0f, // 18
		1.0, 1.0, -1.0, 0, 1,   0.0f,  1.0f,  0.0f, // 19

		// bottom
		-1.0, -1.0, -1.0, 0, 0, 0.0f,  -1.0f,  0.0f, // 20
		1.0, -1.0, -1.0, 1, 0,  0.0f,  -1.0f,  0.0f, // 21
		1.0, -1.0,  1.0, 1, 1,  0.0f,  -1.0f,  0.0f, // 22
		-1.0, -1.0,  1.0, 0, 1, 0.0f,  -1.0f,  0.0f, // 23
	};

	unsigned int indices[] = {
		0,  1,  2,  0,  2,  3,   // front
		4,  5,  6,  4,  6,  7,   // right
		8,  9,  10, 8,  10, 11,  // back
		12, 14, 13, 12, 15, 14,  // left
		16, 18, 17, 16, 19, 18,  // upper
		20, 22, 21, 20, 23, 22   // bottom
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// define position pointer layout 0
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(0 * sizeof(GLfloat)));
	glEnableVertexAttribArray(0);

	// define texcoord pointer layout 1
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	// define normal pointer layout 2
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0);

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

}

void Demo::DrawPapanTulis(
	float translateX, float translateY, float translateZ,
	float scaleX, float scaleY, float scaleZ, GLuint shader
)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texturePapanTulis);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	model = glm::translate(model, glm::vec3(0 + translateX, 1 + translateY, 0 + translateZ));

	model = glm::scale(model, glm::vec3(scaleX, scaleY, scaleZ));

	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildColoredPlane()
{
	// Load and create a texture 
	glGenTextures(1, &texturePlane);
	glBindTexture(GL_TEXTURE_2D, texturePlane);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	int width, height;
	unsigned char* image = SOIL_load_image("lantai.png", &width, &height, 0, SOIL_LOAD_RGBA);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
	glGenerateMipmap(GL_TEXTURE_2D);
	SOIL_free_image_data(image);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Build geometry
	GLfloat vertices[] = {
		// format position, tex coords
		// bottom
		-15.0, -0.5, -15.0,  0,  0, 0.0f,  1.0f,  0.0f,
		 15.0, -0.5, -15.0, 15,  0,	0.0f,  1.0f,  0.0f,
		 15.0, -0.5,  25.0, 15, 15,	0.0f,  1.0f,  0.0f,
		-15.0, -0.5,  25.0,  0, 15,	0.0f,  1.0f,  0.0f,
	};

	GLuint indices[] = { 0,  2,  1,  0,  3,  2 };

	glGenVertexArrays(1, &VAO2);
	glGenBuffers(1, &VBO2);
	glGenBuffers(1, &EBO2);

	glBindVertexArray(VAO2);

	glBindBuffer(GL_ARRAY_BUFFER, VBO2);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO2);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Position attribute
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);
	glEnableVertexAttribArray(0);
	// TexCoord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);
	// Normal attribute
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (GLvoid*)(5 * sizeof(GLfloat)));
	glEnableVertexAttribArray(2);
	// Unbind VAO
	glBindVertexArray(0); 
}

void Demo::DrawColoredPlane(GLuint shader)
{
	UseShader(shader);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texturePlane);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, depthMap);

	glBindVertexArray(VAO2); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized

	glm::mat4 model;
	GLint modelLoc = glGetUniformLocation(shader, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
}

void Demo::BuildDepthMap() {
	// configure depth map FBO
	glGenFramebuffers(1, &depthMapFBO);
	// create depth texture
	glGenTextures(1, &depthMap);
	glBindTexture(GL_TEXTURE_2D, depthMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->SHADOW_WIDTH, this->SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
	glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

	// attach depth texture as FBO's depth buffer
	glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
	glDrawBuffer(GL_NONE);
	glReadBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Demo::BuildShaders()
{
	// build and compile our shader program
	shadowmapShader = BuildShader("shadowMapping.vert", "shadowMapping.frag", nullptr);
	depthmapShader = BuildShader("depthMapping.vert", "depthMapping.frag", nullptr);
}

void Demo::InitCamera()
{
	posCamX = 0.0f;
	posCamY = 2.0f;
	posCamZ = 8.0f;
	viewCamX = 0.0f;
	viewCamY = 1.0f;
	viewCamZ = 0.0f;
	upCamX = 0.0f;
	upCamY = 1.0f;
	upCamZ = 0.0f;
	CAMERA_SPEED = 0.01f;
	fovy = 45.0f;
	glfwSetInputMode(this->window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Demo::MoveCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	// forward positive cameraspeed and backward negative -cameraspeed.
	posCamX = posCamX + x * speed;
	posCamZ = posCamZ + z * speed;
	viewCamX = viewCamX + x * speed;
	viewCamZ = viewCamZ + z * speed;
}

void Demo::StrafeCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	float orthoX = -z;
	float orthoZ = x;

	// left positive cameraspeed and right negative -cameraspeed.
	posCamX = posCamX + orthoX * speed;
	posCamZ = posCamZ + orthoZ * speed;
	viewCamX = viewCamX + orthoX * speed;
	viewCamZ = viewCamZ + orthoZ * speed;
}

void Demo::RotateCamera(float speed)
{
	float x = viewCamX - posCamX;
	float z = viewCamZ - posCamZ;
	viewCamZ = (float)(posCamZ + glm::sin(speed) * x + glm::cos(speed) * z);
	viewCamX = (float)(posCamX + glm::cos(speed) * x - glm::sin(speed) * z);
}

int main(int argc, char** argv) {
	RenderEngine &app = Demo();
	app.Start("Proyek Akhir Sendiri Aja", 1920, 1080, true, true);
}