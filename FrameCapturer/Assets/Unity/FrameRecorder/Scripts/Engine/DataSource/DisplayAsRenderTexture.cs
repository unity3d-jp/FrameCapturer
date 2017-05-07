using System;

namespace UnityEngine.Recorder.FrameRecorder.DataSource
{
    public class DisplayAsRenderTexture : CameraAsRenderTexture
    {
        public int m_DisplayID;

        public void PrepareNewFrame(RecordingSession session)
        {
            if (TargetCamera != null && TargetCamera.targetDisplay != m_DisplayID)
                TargetCamera = null;

            if (TargetCamera == null)
            {
                var displayGO = new GameObject();
                displayGO.name = "CameraHostGO-" + displayGO.GetInstanceID();
                displayGO.transform.parent = session.m_RecorderGO.transform;
                var camera = displayGO.AddComponent<Camera>();
                camera.clearFlags = CameraClearFlags.Nothing;
                camera.cullingMask = 0;
                camera.renderingPath = RenderingPath.DeferredShading;
                camera.targetDisplay = m_DisplayID;
                camera.rect = new Rect(0, 0, 1, 1);
                camera.depth = float.MaxValue;

                TargetCamera = camera;
            }

            base.PrepareNewFrame();
        }

        public DisplayAsRenderTexture(int width)
            : base(width) {}
    }
}
