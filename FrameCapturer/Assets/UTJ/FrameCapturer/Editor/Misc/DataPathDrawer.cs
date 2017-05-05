using UnityEngine;
using UnityEditor;

namespace UTJ.FrameCapturer
{
    [CustomPropertyDrawer(typeof(DataPath))]
    class DataPathDrawer : PropertyDrawer
    {
        public override void OnGUI(Rect position, SerializedProperty property, GUIContent label)
        {
            bool ro = property.FindPropertyRelative("m_readOnly").boolValue;
            if(ro) { EditorGUI.BeginDisabledGroup(true); }

            EditorGUI.BeginProperty(position, label, property);
            position = EditorGUI.PrefixLabel(position, GUIUtility.GetControlID(FocusType.Passive), label);

            var indent = EditorGUI.indentLevel;
            EditorGUI.indentLevel = 0;

            float rootWidth = 70;
            float leafWidth = position.width - rootWidth - 5;
            Rect rootRect = new Rect(position.x, position.y, rootWidth, position.height);
            Rect leafRect = new Rect(position.x + rootWidth + 5, position.y, leafWidth, position.height);

            EditorGUI.PropertyField(rootRect, property.FindPropertyRelative("m_root"), GUIContent.none);
            EditorGUI.PropertyField(leafRect, property.FindPropertyRelative("m_leaf"), GUIContent.none);

            EditorGUI.indentLevel = indent;
            EditorGUI.EndProperty();

            if (ro) { EditorGUI.EndDisabledGroup(); }
        }
    }
}
