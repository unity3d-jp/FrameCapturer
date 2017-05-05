using System;
using UnityEditor;
using UnityEngine;

namespace UTJ
{
    public class MovieRecorderEditorBase : Editor
    {
        public virtual void VideoConfigBasic()
        {

        }

        public virtual void VideoConfigAdvanced()
        {
        }


        public virtual void AudioConfigBasic()
        {

        }

        public virtual void AudioConfigAdvanced()
        {
        }


        public virtual void RecordingControl(MovieRecorderBase recorder)
        {
            if(recorder.isRecording)
            {
                if(GUILayout.Button("Begin Recording"))
                {
                    recorder.BeginRecording();
                }
            }
            else
            {
                if (GUILayout.Button("End Recording"))
                {
                    recorder.EndRecording();
                }
            }
        }


        public override void OnInspectorGUI()
        {
            //DrawDefaultInspector();

            var recorder = target as MovieRecorderBase;

            if (recorder.m_captureVideo)
            {
                VideoConfigBasic();
            }

            if (recorder.m_captureAudio)
            {
                AudioConfigBasic();
            }
        }
    }
}
