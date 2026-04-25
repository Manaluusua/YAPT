from py_yapt import Scene
from yapt.camera_controller import CameraController

class SceneWrapper:
    def __init__(self, renderer):
        self._renderer = renderer
        self._scene = Scene()
        self._scene.init(self._renderer)
        self._cam = CameraController(self._scene.getMainCamera())
        self._objects = set()

    def shutdown(self):
        self._objects.clear()
        self._scene.shutdown()

        
    def get_scene(self):
        return self._scene

    def get_camera_controller(self):
        return self._cam


    def update(self, dt):
        self._cam.update(dt);
        self._scene.update(dt);


    def create_render_object(self, name):
        ro = self.get_scene().createRenderObject(name)
        self._objects.add(ro)
        return ro

    def create_scene_object(self, name):
       ro = self.get_scene().createSceneObject(name)
       self._objects.add(ro)
       return ro

    def destroy_object(self, obj):
        self._objects.remove(obj)

    def get_all_objects_list(self):
        return list(self._objects)

    def get_main_camera(self):
        return self.get_scene().getMainCamera()