#include "framework.h"
#include "parseOSM.hpp"
#include "mercator.h"

#include <filesystem>


#pragma region shaders

static const auto k_fragment_colored_vertex_source = R"(
	#version 330
    precision highp float;

	uniform mat4 MVP;			// Model-View-Projection matrix in row-major format

	layout(location = 0) in vec2 vertexPosition;	// Attrib Array 0
	layout(location = 1) in vec3 vertexColor;	    // Attrib Array 1
	
	out vec3 color;									// output attribute

	void main() {
		color = vertexColor;														// copy color from input to output
		gl_Position = vec4(vertexPosition.x, vertexPosition.y, 0, 1) * MVP; 		// transform to clipping space
	}
)";

static const auto k_fragment_colored_fragment_source = R"(
	#version 330
    precision highp float;

	in vec3 color;				// variable input: interpolated color of vertex shader
	out vec4 fragmentColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() {
		fragmentColor = vec4(color, 1); // extend RGB to RGBA
	}
)";

#pragma endregion


class camera_2d
{
	vec2 _size; // width and height in world coordinates
public:
	vec2 center; // center in world coordinates

	camera_2d() : _size(2, 2), center(0, 0)
	{
	}

	mat4 v() const { return TranslateMatrix(-center); }
	mat4 p() const { return ScaleMatrix(vec2(2 / _size.x, 2 / _size.y)); }

	mat4 v_inverse() const { return TranslateMatrix(center); }
	mat4 p_inverse() const { return ScaleMatrix(vec2(_size.x / 2, _size.y / 2)); }

	void zoom(float s) { _size = _size * s; }
	void setZoom(float newZoom) { _size = { newZoom, newZoom }; }
	void pan(vec2 t) { center = center + t; }
	std::pair<vec2, vec2> getBounds() {
		return std::pair<vec2, vec2>(
			{ center.x - _size.x / 2,center.x + _size.x / 2 },
			{ center.y - _size.y / 2,center.y + _size.y / 2 });
	}
};

camera_2d g_camera; // 2D camera
GPUProgram g_fragment_colored_program;

class mapWrapper
{
	GLuint* _vao{}; // vertex array object
	GLuint* _vbo{}; // vertex buffer object

public:
	MapData mapData;
	std::string userPath = "NOTSET";
private:
	std::map<EWayType, std::vector<colored_point>> vertices;

	void promptPath()
	{
		auto userPath = std::string();

		auto dsa = std::filesystem::directory_iterator(std::filesystem::current_path());
		std::cout << ".osm files in current folder:\n";

		for (const auto& elem : dsa)
			if (dsa->path().extension() == ".osm")
				std::cout << dsa->path().filename() << "\n";

		std::cout << "file to load (default map.osm): ";
		std::cin >> userPath;

		if (userPath.empty())
			userPath = "map.osm";

		this->userPath = userPath;
	}

public:

	void create()
	{
		// Get Map Path
		if (userPath == "NOTSET")
			promptPath();

		// Get Map Data --------
		if (!getNodes(userPath, mapData))
			std::cout << "FAILED TO LOAD\n" << "FAILED TO LOAD\n" << "FAILED TO LOAD\n";


		// Adjust g_camera --------
		g_camera.center = { (float)lon2x_m(((mapData.bounds.minlon + mapData.bounds.maxlon) / 2)), (float)lat2y_m(((mapData.bounds.minlat + mapData.bounds.maxlat) / 2)) };
		g_camera.setZoom((float)lat2y_m((mapData.bounds.maxlat - mapData.bounds.minlat)));



		// --------
		std::cout << "Creating display object...\n";

		std::cout << "vertex count: " << mapData.getNodeCount() << "\n"
			<< "minlat: " << mapData.bounds.minlat
			<< " maxlat: " << mapData.bounds.maxlat
			<< " minlon: " << mapData.bounds.minlon
			<< " maxlon: " << mapData.bounds.maxlon << "\n";


		auto initGL = [](GLuint& vao, GLuint& vbo)
		{
			glGenVertexArrays(1, &vao);
			glBindVertexArray(vao);

			glGenBuffers(1, &vbo); // Generate 1 vertex buffer object
			glBindBuffer(GL_ARRAY_BUFFER, vbo);
			// Enable the vertex attribute arrays
			glEnableVertexAttribArray(0);
			glEnableVertexAttribArray(1);

			glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(colored_point), (void*)offsetof(colored_point, coord));
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(colored_point), (void*)offsetof(colored_point, color));
			// attribute array, components/attribute, component type, normalize?, stride, offset
		};

		_vao = new GLuint[mapData.wayArrs.size()];
		_vbo = new GLuint[mapData.wayArrs.size()];
		std::cout << "Number of buffers: " << mapData.wayArrs.size() << "\n";


		for (const auto& wayArr : mapData.wayArrs)
		{
			vertices.insert({ wayArr.first, {} });
			initGL(_vao[wayArr.first], _vbo[wayArr.first]);
		}


		compile_vertex_data();
	}

	void compile_vertex_data()
	{
		for (const auto& wayArr : mapData.wayArrs)
		{
			//mapData.draw(wayArr.first, _vbo[wayArr.first]);


			vec3 wayColour;
			switch (wayArr.first)
			{
			case EWayType::building:
				wayColour = { .65f, .65f, .65f };
				break;
			case EWayType::route_highway:
				wayColour = { .8f,.8f,.2f };
				break;
			case EWayType::route_railway:
				wayColour = { .6f,.1f,.1f };
				break;
			case EWayType::boundary:
				wayColour = { .05f,.2f,.4f };
				break;
			case EWayType::path:
				wayColour = { .4f,.3f,.1f };
				break;
			case EWayType::grassland:
				wayColour = { .2f, .5f, .3f };
				break;
			case EWayType::sand:
				wayColour = { .8f,.8f,.5f };
				break;
			case EWayType::water:
				wayColour = { .1f,.6f,.8f };
				break;
			case EWayType::defaultt:
				wayColour = { .2f, .2f, .2f };
				break;
			}

			auto findIter = vertices.find(wayArr.first);
			if (findIter != vertices.end())
			{
				auto& vertVector = findIter->second;


				for (const auto& way : wayArr.second)
				{
					if (way.nodes.size() > 0)
					{
						vertVector.push_back(colored_point{ {(float)lon2x_m(way.nodes[0].lon),(float)lat2y_m(way.nodes[0].lat)},wayColour });

						for (const auto& node : way.nodes)
						{
							vertVector.push_back(colored_point{ {(float)lon2x_m(node.lon),(float)lat2y_m(node.lat)},wayColour });
							vertVector.push_back(colored_point{ {(float)lon2x_m(node.lon),(float)lat2y_m(node.lat)},wayColour });
						}

						vertVector.push_back(colored_point{ {(float)lon2x_m(way.nodes[way.nodes.size() - 1].lon),(float)lat2y_m(way.nodes[way.nodes.size() - 1].lat)},wayColour });
					}
				}

				auto baseVector = vertices.find(wayArr.first)->second;

				std::cout << "Writing " << sizeof(colored_point) << " * " << baseVector.size() << " = " << sizeof(colored_point) * baseVector.size() / 1024 << " kB to Buffer " << _vbo[wayArr.first] << "\n";

				glBindBuffer(GL_ARRAY_BUFFER, _vbo[wayArr.first]);
				glBufferData(GL_ARRAY_BUFFER, sizeof(colored_point) * baseVector.size(), baseVector.data(), GL_DYNAMIC_DRAW);
			}
		}
	}

	void draw() const
	{
		g_fragment_colored_program.Use();
		auto mvp_transform = g_camera.v() * g_camera.p();
		mvp_transform.SetUniform(g_fragment_colored_program.getId(), (char*)"MVP");


		for (const auto& vert : vertices)
		{
			auto baseVector = vertices.find(vert.first);
			if (baseVector != vertices.end())
			{
				glBindVertexArray(_vao[vert.first]);
				switch (vert.first)
				{
				case EWayType::path:
					glDrawArrays(GL_LINES, 0, baseVector->second.size());
					break;
				default:
					glDrawArrays(GL_LINES, 0, baseVector->second.size());
					break;
				}
			}
		};
	}
};

mapWrapper g_nodeWrapper;

// Initialization, create an OpenGL context
void onInitialization()
{
	glViewport(0, 0, windowWidth, windowHeight); // Position and size of the photograph on screen
	glLineWidth(2.0f); // Width of lines in pixels

	// create objects by setting up their vertex data on the GPU
	g_nodeWrapper.create();
	GLenum asd = glGetError();

	// create program for the GPU
	g_fragment_colored_program.Create(k_fragment_colored_vertex_source, k_fragment_colored_fragment_source, "fragmentColor");
}


// Window has become invalid: Redraw
void onDisplay()
{
	glClearColor(.4f, .6f, .4f, 0); // background color 
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // clear the screen


	//Draw stuff
	g_nodeWrapper.draw();


	glutSwapBuffers(); // exchange the two buffers
}


#pragma region KeyboardEvents

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY)
{
	switch (key)
	{
	case 'q':
		g_camera.zoom(1.1f);
		break;
	case 'e':
		g_camera.zoom(0.9f);
		break;
	case 'g':
	{
		// Remote data acquisition test
		auto cen = g_camera.center;
		double xd = x2lon_m(cen.x);
		double yd = y2lat_m(cen.y);
		double x = std::round(xd * 100.) / 100.;
		double y = std::round(yd * 100.) / 100.;
		Bounds bs = Bounds(y - 0.005, y + 0.005, x - 0.005, x + 0.005);
		auto data = getOnlineOSM(bs);
		auto md = MapData();
		getNodesFromXML(*data, md);
		md.bounds = bs;
		g_nodeWrapper.mapData.merge(md);

		g_nodeWrapper.compile_vertex_data();
		g_nodeWrapper.draw();

		break;
	}
	}
	glutPostRedisplay();
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY)
{
}

#pragma endregion

#pragma region MouseEvents


bool panning;
vec2 initialpan;
vec2 initialmouse;

// Mouse click event
void onMouse(int button, int state, int pX, int pY)
{
	// GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
	const auto cX = 2.0f * pX / windowWidth - 1; // flip y axis
	const auto cY = 1.0f - 2.0f * pY / windowHeight;

	//std::cout << button;

	if (button == GLUT_LEFT_BUTTON)
	{
		if (state == GLUT_DOWN)
		{
			initialpan = g_camera.center;
			initialmouse = { cX, cY };
			panning = true;
		}
		else
		{
			panning = false;
		}
	}
	else if (button == 3)
		g_camera.zoom(0.9f);
	else if (button == 4)
		g_camera.zoom(1.1f);

	glutPostRedisplay();
}


// Move mouse with key pressed
void onMouseMotion(int pX, int pY)
{
	if (panning)
	{
		// GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON and GLUT_DOWN / GLUT_UP
		const auto cX = 2.0f * pX / windowWidth - 1; // flip y axis
		const auto cY = 1.0f - 2.0f * pY / windowHeight;

		auto bounds = g_camera.getBounds();
		g_camera.center = initialpan + vec2{
				(bounds.first.y - bounds.first.x) / 2 * -(cX - initialmouse.x),
				(bounds.second.y - bounds.second.x) / 2 * -(cY - initialmouse.y) };

		//std::cout << cX << "  " << cY << "\n";

		glutPostRedisplay(); // redraw
	}
}

#pragma endregion

// Idle event indicating that some time elapsed: do animation here
void onIdle()
{
	//glutPostRedisplay();
}
