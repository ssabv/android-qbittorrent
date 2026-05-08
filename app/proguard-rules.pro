-keep class com.qbandroid.** { *; }
-keepclassmembers class * {
    @android.webkit.JavascriptInterface <methods>;
}
-keep class libtorrent.** { *; }
-keep class * extends java.lang.Exception { *; }
-dontwarn org.libtorrent.**
-dontwarn boost.**
-dontwarn ssl.**
-dontwarn asio.**
-dontwarn lt.**

-keepattributes Signature
-keepattributes *Annotation*
-keepattributes EnclosingMethod
-keepattributes InnerClasses

-keep class kotlin.Metadata { *; }
-keep class kotlin.jvm.** { *; }
-keep class androidx.** { *; }
-keep class com.google.** { *; }

-repackageclasses ''
-allowaccessmodification