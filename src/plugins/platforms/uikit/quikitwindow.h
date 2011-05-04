/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QUIKITWINDOW_H
#define QUIKITWINDOW_H

#include <QPlatformWindow>

#import <UIKit/UIKit.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>
#import <OpenGLES/EAGL.h>

@interface EAGLView : UIView <UIKeyInput>
{
    QPlatformWindow *mWindow;
    EAGLContext *mContext;

    GLint mFramebufferWidth;
    GLint mFramebufferHeight;

    GLuint mFramebuffer, mColorRenderbuffer, mDepthRenderbuffer;

    id delegate;
    // ------- Text Input ----------
    UITextAutocapitalizationType autocapitalizationType;
    UITextAutocorrectionType autocorrectionType;
    BOOL enablesReturnKeyAutomatically;
    UIKeyboardAppearance keyboardAppearance;
    UIKeyboardType keyboardType;
    UIReturnKeyType returnKeyType;
    BOOL secureTextEntry;
}

- (void)setContext:(EAGLContext *)newContext;
- (void)presentFramebuffer;
- (void)deleteFramebuffer;
- (void)createFramebuffer;
- (void)makeCurrent;
- (void)setWindow:(QPlatformWindow *)window;
- (void)sendMouseEventForTouches:(NSSet *)touches withEvent:(UIEvent *)event fakeButtons:(Qt::MouseButtons)buttons;

@property (readonly,getter=fbo) GLint fbo;
@property (nonatomic, assign) id delegate;

// ------- Text Input ----------

@property(nonatomic) UITextAutocapitalizationType autocapitalizationType;
@property(nonatomic) UITextAutocorrectionType autocorrectionType;
@property(nonatomic) BOOL enablesReturnKeyAutomatically;
@property(nonatomic) UIKeyboardAppearance keyboardAppearance;
@property(nonatomic) UIKeyboardType keyboardType;
@property(nonatomic) UIReturnKeyType returnKeyType;
@property(nonatomic, getter=isSecureTextEntry) BOOL secureTextEntry;

@end

@protocol EAGLViewDelegate
- (void)eaglView:(EAGLView *)view usesFramebuffer:(GLuint)buffer;
@end

class EAGLPlatformContext;

QT_BEGIN_NAMESPACE

class QUIKitScreen;

class QUIKitWindow : public QPlatformWindow
{
public:
    explicit QUIKitWindow(QWidget *tlw);
    ~QUIKitWindow();

    UIWindow *nativeWindow() const { return mWindow; }
    EAGLView *nativeView() const { return mView; }
    void setGeometry(const QRect &rect);

    UIWindow *ensureNativeWindow();

    QPlatformGLContext *glContext() const;

private:
    QUIKitScreen *mScreen;
    UIWindow *mWindow;
    EAGLView *mView;
    mutable EAGLPlatformContext *mContext;
};

QT_END_NAMESPACE

#endif // QUIKITWINDOW_H
