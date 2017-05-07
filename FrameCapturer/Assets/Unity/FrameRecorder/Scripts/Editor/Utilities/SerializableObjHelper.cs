using System;
using System.Linq.Expressions;

namespace UnityEditor.Recorder.FrameRecorder.Utilities
{
    public static class SerializableObjHelper
    {
        public static SerializedProperty FindProperty(this SerializedObject obj, Expression<Func<object>> exp)
        {
            var body = exp.Body as MemberExpression;
            if (body == null)
            {
                var ubody = (UnaryExpression)exp.Body;
                body = ubody.Operand as MemberExpression;
            }

            var name = body.Member.Name;

            return obj.FindProperty(name);
        }

        public static SerializedProperty FindPropertyRelative(this SerializedProperty obj, Expression<Func<object>> exp)
        {
            var body = exp.Body as MemberExpression;
            if (body == null)
            {
                var ubody = (UnaryExpression)exp.Body;
                body = ubody.Operand as MemberExpression;
            }

            var name = body.Member.Name;

            return obj.FindPropertyRelative(name);
        }
    }
}
