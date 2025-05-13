// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:file-handling

package org.qtproject.qt.android;

import android.content.Context;
import android.content.res.AssetFileDescriptor;
import android.content.res.AssetManager;
import android.os.Build;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;

import java.io.FileInputStream;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

import java.util.zip.ZipFile;
import java.util.Enumeration;
import java.util.zip.ZipEntry;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Comparator;

import android.util.Log;

import java.nio.channels.FileChannel;
import java.nio.channels.FileChannel.MapMode;
import java.nio.MappedByteBuffer;
import java.nio.ByteOrder;

@UsedFromNativeCode
class QtApkFileEngine {
    private final static String QtTAG = QtApkFileEngine.class.getSimpleName();
    private static String m_appApkPath;

    private AssetFileDescriptor m_assetFd;
    private final AssetManager m_assetManager;
    private FileInputStream m_assetInputStream;
    private long m_pos = -1;

    QtApkFileEngine(Context context)
    {
        m_assetManager = context.getAssets();
    }

    boolean open(String fileName)
    {
        try {
            m_assetFd = m_assetManager.openNonAssetFd(fileName);
            m_assetInputStream = m_assetFd.createInputStream();
        } catch (IOException e) {
            Log.e(QtTAG, "Failed to open the app APK with " + e);
        }

        return m_assetInputStream != null;
    }

    boolean close()
    {
        try {
            if (m_assetInputStream != null)
                m_assetInputStream.close();
            if (m_assetFd != null)
                m_assetFd.close();
        } catch (IOException e) {
            Log.e(QtTAG, "Failed to close resources with " + e);
        }

        return m_assetInputStream == null && m_assetFd == null;
    }

    long pos()
    {
        return m_pos;
    }

    boolean seek(int pos)
    {
        if (m_assetInputStream != null && m_assetInputStream.markSupported()) {
            try {
                m_assetInputStream.mark(pos);
                m_assetInputStream.reset();
                m_pos = pos;
                return true;
            } catch (IOException ignored) { }
        }

        return false;
    }

    MappedByteBuffer getMappedByteBuffer(long offset, long size)
    {
        try {
            FileChannel fileChannel = m_assetInputStream.getChannel();
            long position = fileChannel.position() + offset;
            MappedByteBuffer mapped = fileChannel.map(MapMode.READ_ONLY, position, size);
            mapped.order(ByteOrder.LITTLE_ENDIAN);
            fileChannel.close();

            return mapped;
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to map APK file to memory with " + e);
        }

        return null;
    }

    byte[] read(long maxlen)
    {
        if (m_assetInputStream == null)
            return null;

        ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        int bytesRead;
        int totalBytesRead = 0;
        byte[] buffer = new byte[1024];
        try {
            while (totalBytesRead < maxlen) {
                int remainingBytes = (int) maxlen - totalBytesRead;
                int bytesToRead = Math.min(buffer.length, remainingBytes);
                if ((bytesRead = m_assetInputStream.read(buffer, 0, bytesToRead)) == -1)
                    break;
                outputStream.write(buffer, 0, bytesRead);
                totalBytesRead += bytesRead;
            }

            outputStream.close();
        } catch (IOException e) {
            Log.e(QtTAG, "Failed to read content with " + e);
        }

        return outputStream.toByteArray();
    }

    static String getAppApkFilePath()
    {
        if (m_appApkPath != null)
            return m_appApkPath;

        try {
            Context context = QtNative.getContext();
            PackageManager pm = context.getPackageManager();
            ApplicationInfo applicationInfo = pm.getApplicationInfo(context.getPackageName(), 0);
            if (applicationInfo.splitSourceDirs != null) {
                m_appApkPath = Arrays.stream(applicationInfo.splitSourceDirs)
                    .filter(file -> Arrays.stream(Build.SUPPORTED_ABIS)
                        .anyMatch(abi -> file.endsWith(abi.replace('-', '_') + ".apk")))
                    .findFirst()
                    .orElse(null);

                if (m_appApkPath == null)
                    Log.d(QtTAG, "No ABI specific split APK found, defaulting to the main APK.");
            }

            if (m_appApkPath == null)
                m_appApkPath = applicationInfo.sourceDir;
        } catch (PackageManager.NameNotFoundException e) {
            Log.e(QtTAG, "Failed to get the app APK path with " + e);
            return null;
        }
        return m_appApkPath;
    }

    static class JFileInfo
    {
        String relativePath;
        boolean isDir;
        long size;
    }

    static ArrayList<JFileInfo> getApkFileInfos(String apkPath)
    {
        ArrayList<JFileInfo> fileInfos = new ArrayList<>();
        HashSet<String> dirSet = new HashSet<>();
        HashSet<String> allDirsSet = new HashSet<>();

        try (ZipFile zipFile = new ZipFile(apkPath)) {
            Enumeration<? extends ZipEntry> enumerator = zipFile.entries();
            while (enumerator.hasMoreElements()) {
                ZipEntry entry = enumerator.nextElement();
                String name = entry.getName();

                // Limit the listing to lib directory
                if (name.startsWith("lib/")) {
                    JFileInfo info = new JFileInfo();
                    info.relativePath = name;
                    info.isDir = entry.isDirectory();
                    info.size = entry.getSize();
                    fileInfos.add(info);

                    // check directories
                    dirSet.add(name.substring(0, name.lastIndexOf("/") + 1));
                }
            }

            // ZipFile iterator doesn't seem to add directories, so add them manually.
            for (String path : dirSet) {
                int index = 0;
                while ((index = path.indexOf("/", index + 1)) != -1) {
                    String dir = path.substring(0, index);
                    allDirsSet.add(dir);
                }
            }

            for (String dir : allDirsSet) {
                JFileInfo info = new JFileInfo();
                info.relativePath = dir;
                info.isDir = true;
                info.size = -1;
                fileInfos.add(info);
            }

            // sort alphabetically based on the file path
            fileInfos.sort(Comparator.comparing(info -> info.relativePath));
        } catch (Exception e) {
            Log.e(QtTAG, "Failed to list App's APK files with " + e);
        }

        return fileInfos;
    }
}
