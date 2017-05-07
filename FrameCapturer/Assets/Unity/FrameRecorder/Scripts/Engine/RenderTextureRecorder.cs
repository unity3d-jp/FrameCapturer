using System.Collections.Generic;
#if UNITY_EDITOR
using UnityEditor;
#endif
using UnityEngine.Recorder.FrameRecorder.DataSource;

namespace UnityEngine.Recorder.FrameRecorder
{
    public abstract class RenderTextureRecorder<T> : Recorder where T : ImageRecorderSettings
    {
        [SerializeField]
        protected T m_Settings;
        public override FrameRecorderSettings settings
        {
            get { return m_Settings; }
            set { m_Settings = (T)value; }
        }

        protected string m_OutputFile;

#if UNITY_EDITOR
        public static EditorWindow GetMainGameView()
        {
            System.Type T = System.Type.GetType("UnityEditor.GameView,UnityEditor");
            System.Reflection.MethodInfo GetMainGameView = T.GetMethod("GetMainGameView", System.Reflection.BindingFlags.NonPublic | System.Reflection.BindingFlags.Static);
            System.Object Res = GetMainGameView.Invoke(null, null);
            return (EditorWindow)Res;
        }

#endif

        public override bool BeginRecording(RecordingSession session)
        {
            base.BeginRecording(session);

            m_BoxedSources = new List<BoxedSource>();

            // Targetting explicit cameras?
            if (session.m_ObjsOfInterest != null)
            {
                foreach (var objOfInterest in session.m_ObjsOfInterest)
                {
                    if (!(objOfInterest is Camera))
                        continue;

                    var source = new CameraAsRenderTexture(m_Settings.m_Width)
                    {
                        TargetCamera = (Camera)objOfInterest
                    };
                    var boxedSource = new BoxedSource(source);
                    boxedSource.m_StageHandlers.Add(ERecordingSessionStage.NewFrameStarting, (x) => source.PrepareNewFrame());

                    m_BoxedSources.Add(boxedSource);
                }
            }

            // Targetting a game display?
            if (m_Settings.m_InputType == EImageSourceType.GameDisplay)
            {
                if (!m_Settings.m_ScaleImage)
                {
#if UNITY_EDITOR
                    if (m_Settings.m_ScreenID == 0)
                        m_Settings.m_Width = (int)GetMainGameView().position.width;
                    else
#endif
                    m_Settings.m_Width = Display.displays[m_Settings.m_ScreenID].renderingWidth;
                }

                var source = new DisplayAsRenderTexture(m_Settings.m_Width);
                var boxedSource = new BoxedSource(source);
                boxedSource.m_StageHandlers.Add(ERecordingSessionStage.NewFrameStarting,
                    (x) =>
                    {
                        source.m_DisplayID = m_Settings.m_ScreenID;
                        source.PrepareNewFrame(session);
                    });

                m_BoxedSources.Add(boxedSource);
            }


            // Targetting the scene view?
#if UNITY_EDITOR
            if (m_Settings.m_InputType == EImageSourceType.SceneView)
            {
                if (!m_Settings.m_ScaleImage)
                {
                    m_Settings.m_Width = (int)SceneView.currentDrawingSceneView.position.width;
                }

                var source = new DisplayAsRenderTexture(m_Settings.m_Width);
                var boxedSource = new BoxedSource(source);
                boxedSource.m_StageHandlers.Add(ERecordingSessionStage.NewFrameStarting,
                    (x) =>
                    {
                        source.m_DisplayID = m_Settings.m_ScreenID;
                        source.PrepareNewFrame(session);
                    });

                m_BoxedSources.Add(boxedSource);
            }
#endif

            // Targetting "Main Camera"
            if (m_Settings.m_InputType == EImageSourceType.MainCamera)
            {
                var source = new CameraAsRenderTexture(m_Settings.m_Width);
                var boxedSource = new BoxedSource(source);
                boxedSource.m_StageHandlers.Add(ERecordingSessionStage.NewFrameStarting,
                    (x) =>
                    {
                        source.TargetCamera = Camera.main;
                        source.PrepareNewFrame();
                    });

                m_BoxedSources.Add(boxedSource);
            }

            // Targetting a "tagged Camera"
            if (m_Settings.m_InputType == EImageSourceType.TaggedCamera)
            {
                var source = new CameraAsRenderTexture(m_Settings.m_Width);
                var boxedSource = new BoxedSource(source);
                boxedSource.m_StageHandlers.Add(ERecordingSessionStage.NewFrameStarting,
                    (x) =>
                    {
                        var candidates = GameObject.FindGameObjectsWithTag(m_Settings.m_CameraTag);
                        if (candidates.Length > 0)
                        {
                            foreach (var candidate in candidates)
                            {
                                var cam = candidate.GetComponent<Camera>();
                                if (cam != null)
                                {
                                    source.TargetCamera = cam;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            Debug.LogError("No GameObject found with tag: " + m_Settings.m_CameraTag);
                        }
                        source.PrepareNewFrame();
                    });

                m_BoxedSources.Add(boxedSource);
            }

            return recording = true;
        }
    }
}
