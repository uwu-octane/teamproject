#include "glm_include.h"
#include "rendergraph.h"

#include "scene_config.h"
#include "scene_config_parser.h"

#define TRANSFORMATION_INTERFACE_IMPL
#include "../../scene_outliner/src/transform_hierarchy.h"

#include "audio.h"
#include <iostream>
#include <thread>
#include <filesystem>
#include <codecvt>
#include <math.h>

namespace
{
struct
{
	float      height = 0;
	AudioState audioState;
	Array<String>     audiopaths;
	Array<String>     node_names;
} settings;

struct sceneObject
{
	uint32_t  objIdx;
	glm::vec3 startingPosition;
};

struct AudioObject
{
	String    objname;
	String    soundname;
	uint32_t  objIdx;
	uint32_t  soundIdx;
	glm::vec3 startingPosition;
	glm::vec3 lastPosition;
	glm::vec3 lastVelocity;
};

float              speed        = 0.005f;
bool               rotating     = false;
float							currentPosOnCircle = 0.00f;
glm::vec3					middle;
Array<sceneObject> sceneObjects = {};
Array<AudioObject> audioObjects = {};

// This function updates the draw information so that the correct mesh
// is rendered
void update_indirect_buffer(App                     &app,
                            RenderGraph::AssetNode  &assetNode,
                            Scene                   &scene,
                            RenderGraph::BufferNode &indirectBufferNode,
                            const SceObject         &obj)
{
	BufferMapper mapper(indirectBufferNode.buffers[0]);
	auto        *data            = (VkDrawIndexedIndirectCommand *)mapper.data;
	MeshDesc    &mesh            = app.meshes[obj.dataIdx];
	data[obj.dataIdx].indexCount = mesh.indexCount;
	data[obj.dataIdx].firstIndex = mesh.firstIndex;
	data[obj.dataIdx].instanceCount = 1;
	data[obj.dataIdx].vertexOffset  = 0;
	data[obj.dataIdx].firstInstance = 0;
}

// this updates all the necessary GPU information. It updates the draw
// information and transformations. Just call this, whenever you add
// another mesh. See the example at the end.
void update_gpu_data(App                &app,
                     RenderGraph::Graph &graph,
                     Renderer           &renderer,
                     const SceObject    &obj)
{
	auto &scene              = app.scenes.sub(app.active_scene);
	auto &assetNode          = graph.getAssetNode("scene_geometry/scene");
	auto &indirectBufferNode = graph.getBufferNode("scene_geometry/indirect");
	auto &transBufferNode =
	    graph.getBufferNode("scene_geometry/transformations");
	update_indirect_buffer(app, assetNode, scene, indirectBufferNode, obj);

	{
		BufferMapper mapper(transBufferNode.buffers[0]);
		glm::mat4   *meshTransformations = (glm::mat4 *)mapper.data;
		Transformation::apply_xforms(app,
		                             graph,
		                             renderer,
		                             obj.idx,
		                             meshTransformations,
		                             nullptr,
		                             nullptr,
		                             glm::mat4(1));
	}
}

void initObject(const String &name,
                const String &audiopath,
                const Scene  &scene,
                App          &app)

{
	for (auto const &x : scene.objects.keys) { printf("%s\n", x.begin());
	}
	if (!scene.objects.map.contains(name)) {
		fprintf(stderr, "Object with name %s does not exist.\n", name.begin());
		assert(false);
		exit(EXIT_FAILURE);
	}

	uint32_t  objIndex    = scene.objects.map[name];

	glm::mat4 cameratrans = glm::inverse(
	    scene.transformations
	        [scene.objects.sub(app.cameras[scene.camIdx].objIdx).idx]);

	glm::vec4 pos =
	    cameratrans * scene.transformations[scene.objects.sub(objIndex).idx][3];

	glm::vec3 lastPosition = pos.xyz();

	std::string path(audiopath.begin(), audiopath.end());
	int position = path.find_last_of("/");

	audioObjects.add({name,
	                  String(path.substr(position + 1).c_str()), 
	                  objIndex,
	                  init_local_sound(audiopath),
					lastPosition,
	                  lastPosition,
	                  glm::vec3(0)});
};

void updateAudioObjects(Scene &scene, App &app)
{
	glm::mat4 cameratrans = glm::inverse(
	    scene.transformations
	        [scene.objects.sub(app.cameras[scene.camIdx].objIdx).idx]);

	for (auto &audioObject : audioObjects) {
		glm::vec4 pos =
		    cameratrans *
		    scene.transformations[scene.objects.sub(audioObject.objIdx).idx][3];

		audioObject.lastVelocity = pos.xyz() - audioObject.lastPosition;
		audioObject.lastPosition = pos.xyz();
		set_sound_position(audioObject.lastPosition.x,
		                   audioObject.lastPosition.y,
		                   audioObject.lastPosition.z, audioObject.soundIdx);
		set_sound_velocity(audioObject.lastVelocity.x,
		                   audioObject.lastVelocity.y,
		                   audioObject.lastVelocity.z,
		                   audioObject.soundIdx);
	}
};
float calcRadius(float x, float z){
	x = x - middle.x;
	z = z - middle.z;
	return sqrt(x*x + z*z);
}
float calcAngle(float radius, float x){
	return acos((x - middle.x)/radius);
}

void moveObjectsCircular(Scene &scene,
                         App &app,
                         RenderGraph::Graph &graph,
                         Renderer &renderer,
                         glm::mat4 *meshTransformations)
{ 
	for (auto &object : sceneObjects) { 
		float radius = calcRadius(object.startingPosition.x, object.startingPosition.z);
		float sign = 0;
		float angle = - calcAngle(radius, object.startingPosition.x) + currentPosOnCircle;
		if(middle.z - object.startingPosition.z < 0){
			angle  = - angle + 2 * currentPosOnCircle;
		}
		//angle = angle + sign;
		scene.transformations[object.objIdx][3].x =
			middle.x + cos(angle) * radius;
		scene.transformations[object.objIdx][3].z =
			middle.z + sin(angle) * radius;
		Transformation::apply_xforms(app,
		 	                             graph,
			                             renderer,
			                             object.objIdx,
			                             meshTransformations,
			                             nullptr,
			                             nullptr,
			                             glm::mat4(1));
	}
};

}  // namespace

extern "C" {
void MT_DLL_EXPORT init(App                &app,
                        GLFWwindow         *window,
                        RenderGraph::Graph &graph,
                        Renderer           &renderer,
                        const SceneConfig  &cfg);

void MT_DLL_EXPORT update(App                &app,
                          RenderGraph::Graph &graph,
                          Renderer           &renderer,
                          uint32_t            idx,
                          float               delta);

void MT_DLL_EXPORT destruct();

void               init(App                &app,
          GLFWwindow         *window,
          RenderGraph::Graph &graph,
          Renderer           &renderer,
          const SceneConfig  &cfg)

{
	 Scene &scene = app.scenes.sub(app.active_scene);
	 TRANSFORMATION_INIT_INTERFACE(app);
	
	 unsigned int count = 0;
	 for (auto &node : cfg.nodes) {
		if (node.type_string != "AudioObject") { continue; }
		String audiopath;
		settings.node_names.add(node.name); 
		for (auto kv : node.settings.list) {
			if (kv.key == "audiopath") { 
				settings.audiopaths.add(kv.value);
				audiopath          = kv.value;
			}
		}
		count++;
	}
	init_audio(0, 48000, count);
}

void update(App                &app,
            RenderGraph::Graph &graph,
            Renderer           &renderer,
            uint32_t            idx,
            float               delta)
{

	Scene     &scene  = app.scenes.sub(app.active_scene);

	auto &transBufferNode =
	    graph.getBufferNode("scene_geometry/transformations");
	{
		BufferMapper mapper(transBufferNode.buffers[0]);
		glm::mat4   *meshTransformations = (glm::mat4 *)mapper.data;

		//// Hier geht Teamprojekt los

		if (ImGui::Begin("Properties")) {
			if (ImGui::CollapsingHeader("Teamprojekt")) {
				static float pos_x;
				static float pos_y;
				static float pos_z;
			   
				if (ImGui::ListBoxHeader("Sound List", audioObjects.size())) {
					for (int i = 0; i < audioObjects.size(); i++) {
						if (&audioObjects[i]) {
							String temp =
							    String(std::to_string(audioObjects[i].soundIdx).c_str());
							ImGui::Selectable(temp.begin(), true);
							ImGui::SameLine();
							ImGui::Selectable(audioObjects[i].soundname.begin(),
							                  true);
							ImGui::SameLine();
							String rad = String(
								std::to_string(calcRadius(audioObjects[i].startingPosition.x, audioObjects[i].startingPosition.z))
								.c_str());
							ImGui::Selectable(rad.begin(), true);

							ImGui::SameLine();
							if (is_playing(i)) {
								ImGui::Selectable(
								    "playing", true);
							} else {
								ImGui::Selectable("not playing", true);
							}
							ImGui::SameLine();
							String positionx =
							    String(std::to_string(audioObjects[i].lastPosition.x)
							        .c_str());
							ImGui::Selectable(positionx.begin(), true);
							ImGui::SameLine();
							String positiony = String(
							    std::to_string(audioObjects[i].lastPosition.y)
							        .c_str());
							ImGui::Selectable(positiony.begin(), true);
							ImGui::SameLine();
							String positionz = String(
							    std::to_string(audioObjects[i].lastPosition.z)
							        .c_str());
							ImGui::Selectable(positionz.begin(), true);
						}
					}
					ImGui::ListBoxFooter();
				}

				//ImGui::InputText("Sound Path",
				//                 settings.audiopath.begin(),
				//                 settings.audiopath.capacity());
				//settings.audiopath.updateSize();
				//ImGui::SameLine();
				//if (ImGui::Button("open local sound")) {
				//	if (settings.audiopath != "") {
				//		auto soundidx = init_local_sound(settings.audiopath);
				//	}
				//}

				static int index = 0;
				ImGui::InputInt("Sound Index", &index);
				if (ImGui::Button("play")) {
					if (index >= 0) {
						play(index);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("play all")) { play_all(); }
				ImGui::SameLine();
				if (ImGui::Button("stop")) { stop(index); }
				ImGui::SameLine();
				if (ImGui::Button("stop all")) { stop_all(); }

				listener_set_position(
				    0, 0, 0);
				listener_set_direction(0, 0, -1);

				static float volume;
				if (ImGui::DragFloat("Volume", &volume, 1.0f, 0.f, 100.f)) {
					set_volume(volume, index);
				}
				static float volume_for_all;
				if (ImGui::DragFloat(
				        "Volume for all", &volume_for_all, 1.0f, 0.f, 100.f)) {
					set_volume_for_all(volume_for_all);
				}

				ImGui::InputFloat("speed", &speed);

				if (ImGui::Button("enable/disable rotating objects")) {
					rotating = !rotating;
				}
				ImGui::SameLine();
				if (rotating) {
					ImGui::Selectable("enabled", true);
				} else {
					ImGui::Selectable("disabled", true);
				}
			}
			ImGui::End();

			if (rotating) {
				moveObjectsCircular(scene,
				                    app,
				                    graph,
				                    renderer,
				                    meshTransformations);
			currentPosOnCircle += speed;
			}
		}

		// This here is just example code. The important bit is, that you
		// call add_mesh, to add an object The mesh that is passed to
		// add_mesh is taken from a mesh that is already added as another
		// object. Reusing that is fine and essentially creates a clone of
		// the previous object.  We use the transformation of the original
		// object and slightly change it, by offsetting it in x direction.
		// For materials you can do something similar to give variation.
		static bool initialized = false;
		if (!initialized) {
			initialized = true;
			// get the object we want to copy
			auto     &scene       = app.scenes.sub(app.active_scene);
			//TRANSFORMATION_INIT_INTERFACE(app);
			
			for (int i = 0; i < settings.audiopaths.size(); i++) {
				initObject(
				    settings.node_names[i], settings.audiopaths[i], scene, app);
			}
			for (auto const& x : scene.objects.keys) {
				printf("%s\n", x.begin());
			}

			 for(int i = 0; i < scene.objects.keys.size(); i++){
				 if(scene.objects.keys[i].contains("PointLight") || scene.objects.keys[i].contains("Camera")){
						continue;
				 }
			 		uint32_t index = i;
			 		glm::vec3 start = scene.transformations[scene.objects.sub(i).idx][3].xyz();
			 		sceneObjects.add({index, start});
			 }
			
			 // uint32_t  cloneObject = 0;
			 // auto     &obj         = scene.objects.sub(cloneObject);
			 // glm::mat4 trans       = scene.transformations[obj.idx];
			 // 
			 // auto mesh = app.meshes[obj.dataIdx];
			 // 
			 // trans[3].x += 2;
			 // add_mesh(app,
			 //          scene,
			 //          mesh,
			 //          // Objects get unique names by default. If this
			 //          // wouldn't be here, the name would automatically get
			 //          // an increasing number at the end.
			 //          (scene.objects.keys[obj.idx] + "Clone").build(),
			 //          trans);
			 // // update_data needs to be called after each add_mesh to
			 // // correctly update the GPU data.
			 // update_gpu_data(app, graph, renderer, scene.objects.list.last());
			 // trans[3].x += 2; 

			 //calculate the middle of the objects for the circular movement
			float maxDistance = 0;
			 for (auto& object1 : sceneObjects) {
				 for (auto& object2 : sceneObjects) {
					float x = (scene.transformations[object1.objIdx][3].x - scene.transformations[object2.objIdx][3].x);
					float z = (scene.transformations[object1.objIdx][3].z - scene.transformations[object2.objIdx][3].z);

					float distance = sqrt(x*x + z*z);
					if(distance > maxDistance){
						maxDistance = distance;
						middle = (scene.transformations[object1.objIdx][3] + scene.transformations[object2.objIdx][3]);
						middle.x /= 2;
						middle.z/= 2;
					}
				
				 }
			 }
		}
	 }
	updateAudioObjects(scene, app);
}
void destruct() { cleanup_audio(); }
}
