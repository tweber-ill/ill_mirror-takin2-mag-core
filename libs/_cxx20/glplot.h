/**
 * GL plotter
 * @author Tobias Weber <tweber@ill.fr>
 * @date Nov-2017 -- 2019
 * @license GPLv3, see 'LICENSE' file
 * @desc The present version was forked on 8-Nov-2018 from the privately developed "magtools" project (https://github.com/t-weber/magtools).
 */

#ifndef __MAG_GL_PLOT_H__
#define __MAG_GL_PLOT_H__

#include <QtCore/QThread>
#include <QtCore/QMutex>

#include <QtCore/QTimer>
#include <QtWidgets/QDialog>
#include <QtGui/QMouseEvent>

#include <QtGui/QOpenGLShaderProgram>
#include <QtGui/QOpenGLBuffer>
#include <QtWidgets/QOpenGLWidget>

#include <QtGui/QMatrix4x4>
#include <QtGui/QVector4D>
#include <QtGui/QVector3D>

#include <memory>
#include <chrono>
#include <atomic>

#include "math_algos.h"



// ----------------------------------------------------------------------------
// GL versions
#if !defined(_GL_MAJ_VER) || !defined(_GL_MIN_VER)
	#define _GL_MAJ_VER 3
	#define _GL_MIN_VER 3
#endif

// GL functions include
#define _GL_INC_IMPL(MAJ, MIN) <QtGui/QOpenGLFunctions_ ## MAJ ## _ ## MIN ## _Core>
#define _GL_INC(MAJ, MIN) _GL_INC_IMPL(MAJ, MIN)
#include _GL_INC(_GL_MAJ_VER, _GL_MIN_VER)

// GL functions typedef
#define _GL_FUNC_IMPL(MAJ, MIN) QOpenGLFunctions_ ## MAJ ## _ ## MIN ## _Core
#define _GL_FUNC(MAJ, MIN) _GL_FUNC_IMPL(MAJ, MIN)
using qgl_funcs = _GL_FUNC(_GL_MAJ_VER, _GL_MIN_VER);

// GL surface format
extern void set_gl_format(bool bCore=true, int iMajorVer=3, int iMinorVer=3, int iSamples=8);

// GL error codes: https://www.khronos.org/opengl/wiki/OpenGL_Error
#define LOGGLERR(pGl) { while(true) {	\
		auto err = pGl->glGetError();	\
		if(err == GL_NO_ERROR) break;	\
		std::cerr << "gl error in " << __func__ << " line " << std::dec <<  __LINE__  << ": " << std::hex << err << std::endl; \
	}}
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// types
using t_real_gl = GLfloat;
using t_vec3_gl = m::qvecN_adapter<int, 3, t_real_gl, QVector3D>;
using t_vec_gl = m::qvecN_adapter<int, 4, t_real_gl, QVector4D>;
using t_mat_gl = m::qmatNN_adapter<int, 4, 4, t_real_gl, QMatrix4x4>;

// forward declarations
class GlPlot_impl;
class GlPlot;
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// plotter objects
enum class GlPlotObjType
{
	TRIANGLES,
	LINES
};


struct GlPlotObj
{
	// does not define a geometry itself, but just links to another object
	std::optional<std::size_t> linkedObj;

	GlPlotObjType m_type = GlPlotObjType::TRIANGLES;
	GLuint m_vertexarr = 0;

	std::shared_ptr<QOpenGLBuffer> m_pvertexbuf;
	std::shared_ptr<QOpenGLBuffer> m_pnormalsbuf;
	std::shared_ptr<QOpenGLBuffer> m_pcolorbuf;

	std::vector<t_vec3_gl> m_vertices, m_triangles;
	t_vec_gl m_color = m::create<t_vec_gl>({ 0., 0., 1., 1. });	// rgba

	t_mat_gl m_mat = m::unit<t_mat_gl>();

	bool m_invariant = false;	// invariant to A, B matrices
	bool m_visible = true;		// object shown?
	bool m_valid = true;		// object deleted?

	t_vec3_gl m_labelPos = m::create<t_vec3_gl>({0., 0., 0.});
	std::string m_label;
};
// ----------------------------------------------------------------------------




// ----------------------------------------------------------------------------
/**
 * GL plotter implementation
 */
class GlPlot_impl : public QObject
{ Q_OBJECT
private:
	QMutex m_mutexObj{QMutex::Recursive};

protected:
	GlPlot *m_pPlot = nullptr;
	std::string m_strGlVer, m_strGlShaderVer, m_strGlVendor, m_strGlRenderer;

	std::shared_ptr<QOpenGLShaderProgram> m_pShaders;

	GLint m_attrVertex = -1;
	GLint m_attrVertexNorm = -1;
	GLint m_attrVertexCol = -1;
	GLint m_uniConstCol = -1;
	GLint m_uniLightPos = -1;
	GLint m_uniNumActiveLights = -1;
	GLint m_uniMatrixProj = -1;
	GLint m_uniMatrixCam = -1;
	GLint m_uniMatrixCamInv = -1;
	GLint m_uniMatrixObj = -1;
	GLint m_uniMatrixA = -1;
	GLint m_uniMatrixB = -1;
	GLint m_uniCoordSys = -1;

	t_mat_gl m_matPerspective = m::unit<t_mat_gl>();
	t_mat_gl m_matPerspective_inv = m::unit<t_mat_gl>();
	t_mat_gl m_matViewport = m::unit<t_mat_gl>();
	t_mat_gl m_matViewport_inv = m::unit<t_mat_gl>();
	t_mat_gl m_matCamBase = m::create<t_mat_gl>({1,0,0,0,  0,1,0,0,  0,0,1,-5,  0,0,0,1});
	t_mat_gl m_matCamRot = m::unit<t_mat_gl>();
	t_mat_gl m_matCam = m::unit<t_mat_gl>();
	t_mat_gl m_matCam_inv = m::unit<t_mat_gl>();
	t_mat_gl m_matA = m::unit<t_mat_gl>();
	t_mat_gl m_matB = m::unit<t_mat_gl>();
	t_vec_gl m_vecCamX = m::create<t_vec_gl>({1.,0.,0.,0.});
	t_vec_gl m_vecCamY = m::create<t_vec_gl>({0.,1.,0.,0.});
	t_real_gl m_phi_saved = 0, m_theta_saved = 0;
	t_real_gl m_zoom = 1.;
	t_real_gl m_CoordMax = 2.5;		// extent of coordinate axes

	std::atomic<bool> m_bPlatformSupported = true;
	std::atomic<bool> m_bInitialised = false;
	std::atomic<bool> m_bWantsResize = false;
	std::atomic<bool> m_bPickerNeedsUpdate = false;
	std::atomic<bool> m_bLightsNeedUpdate = false;
	std::atomic<bool> m_bBTrafoNeedsUpdate = false;
	std::atomic<int> m_iCoordSys = 0;
	std::atomic<int> m_iScreenDims[2] = { 800, 600 };
	t_real_gl m_pickerSphereRadius = 1;

	std::vector<t_vec3_gl> m_lights;
	std::vector<GlPlotObj> m_objs;

	QPointF m_posMouse;
	QPointF m_posMouseRotationStart, m_posMouseRotationEnd;
	bool m_bInRotation = false;

	QTimer m_timer;

protected:
	qgl_funcs* GetGlFunctions(QOpenGLWidget *pWidget = nullptr);

	void UpdateCam();
	void RequestPlotUpdate();
	void UpdatePicker();
	void UpdateLights();
	void UpdateBTrafo();

	void DoPaintGL(qgl_funcs *pGL);
	void DoPaintNonGL(QPainter &painter);

	void tick(const std::chrono::milliseconds& ms);

public:
	GlPlot_impl(GlPlot *pPlot = nullptr);
	virtual ~GlPlot_impl();

	static constexpr bool m_isthreaded = false;
	static constexpr bool m_usetimer = false;

	std::tuple<std::string, std::string, std::string, std::string>
		GetGlDescr() const { return std::make_tuple(m_strGlVer, m_strGlShaderVer, m_strGlVendor, m_strGlRenderer); }

	QPointF GlToScreenCoords(const t_vec_gl& vec, bool *pVisible=nullptr);
	static t_mat_gl GetArrowMatrix(const t_vec_gl& vecTo, 
		t_real_gl postscale = 1,  const t_vec_gl& vecPostTrans = m::create<t_vec_gl>({0,0,0.5}), 
		const t_vec_gl& vecFrom = m::create<t_vec_gl>({0,0,1}),
		t_real_gl prescale = 1,  const t_vec_gl& vecPreTrans = m::create<t_vec_gl>({0,0,0}));

	void SetCamBase(const t_mat_gl& mat, const t_vec_gl& vecX, const t_vec_gl& vecY)
	{ m_matCamBase = mat; m_vecCamX = vecX; m_vecCamY = vecY; UpdateCam(); }
	void SetPickerSphereRadius(t_real_gl rad) { m_pickerSphereRadius = rad; }

	GlPlotObj CreateTriangleObject(const std::vector<t_vec3_gl>& verts,
		const std::vector<t_vec3_gl>& triag_verts, const std::vector<t_vec3_gl>& norms,
		const t_vec_gl& color, bool bUseVertsAsNorm=false);
	GlPlotObj CreateLineObject(const std::vector<t_vec3_gl>& verts, const t_vec_gl& color);

	std::size_t GetNumObjects() const { return m_objs.size(); }
	void RemoveObject(std::size_t obj);
	std::size_t AddLinkedObject(std::size_t linkTo,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=1, t_real_gl g=1, t_real_gl b=1, t_real_gl a=1);
	std::size_t AddSphere(t_real_gl rad=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddCylinder(t_real_gl rad=1, t_real_gl h=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddCone(t_real_gl rad=1, t_real_gl h=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddArrow(t_real_gl rad=1, t_real_gl h=1,
		t_real_gl x=0, t_real_gl y=0, t_real_gl z=0,
		t_real_gl r=0, t_real_gl g=0, t_real_gl b=0, t_real_gl a=1);
	std::size_t AddCoordinateCross(t_real_gl min, t_real_gl max);

	void SetObjectMatrix(std::size_t idx, const t_mat_gl& mat);
	void SetObjectCol(std::size_t idx, t_real_gl r, t_real_gl g, t_real_gl b, t_real_gl a=1);
	void SetObjectLabel(std::size_t idx, const std::string& label);
	void SetObjectVisible(std::size_t idx, bool visible);

	void SetScreenDims(int w, int h);
	void SetCoordMax(t_real_gl d) { m_CoordMax = d; }

	void SetLight(std::size_t idx, const t_vec3_gl& pos);

	void SetBTrafo(const t_mat_gl& matB, const t_mat_gl* matA = nullptr);
	void SetCoordSys(int iSys);

public /*slots*/:
	void paintGL();

	void startedThread();
	void stoppedThread();

	void initialiseGL();
	void resizeGL();

	void mouseMoveEvent(const QPointF& pos);
	void zoom(t_real_gl val);
	void ResetZoom();

	void BeginRotation();
	void EndRotation();

protected slots:
	void tick();

signals:
	void PickerIntersection(const t_vec3_gl* pos, std::size_t objIdx, const t_vec3_gl* posSphere);
};



/**
 * GL plotter widget
 */
class GlPlot : public QOpenGLWidget
{ Q_OBJECT
public:
	GlPlot(QWidget *pParent = nullptr);
	virtual ~GlPlot();

	GlPlot_impl* GetImpl() { return m_impl.get(); }
	static constexpr bool m_isthreaded = GlPlot_impl::m_isthreaded;

protected:
	virtual void paintEvent(QPaintEvent*) override;
	virtual void initializeGL() override;
	virtual void paintGL() override;
	virtual void resizeGL(int w, int h) override;

	virtual void mouseMoveEvent(QMouseEvent *pEvt) override;
	virtual void mousePressEvent(QMouseEvent *Evt) override;
	virtual void mouseReleaseEvent(QMouseEvent *Evt) override;
	virtual void wheelEvent(QWheelEvent *pEvt) override;

protected slots:
	void beforeComposing();
	void afterComposing();
	void beforeResizing();
	void afterResizing();

private:
	mutable QMutex m_mutex{QMutex::Recursive};
	std::unique_ptr<GlPlot_impl> m_impl;
	std::unique_ptr<QThread> m_thread_impl;

public:
	QMutex* GetMutex() { return &m_mutex; }

	void MoveContextToThread();
	bool IsContextInThread() const;

signals:
	void AfterGLInitialisation();
	void MouseDown(bool left, bool mid, bool right);
	void MouseUp(bool left, bool mid, bool right);
};
// ----------------------------------------------------------------------------


#endif
