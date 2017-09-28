// TPadManipulator.h
//
// David Adams
// November 2015
//
// Root macros to manipulate a root drawing pad.
//

#ifndef TPadManipulator_H
#define TPadManipulator_H

#include <vector>

class TVirtualPad;
class TH1;
class TLine;

class TPadManipulator {

public:

  // Ctor from a pad. If null the current pad is used.
  TPadManipulator(TVirtualPad* ppad =nullptr);

  // Dtor.
  // This removes the lines.
  ~TPadManipulator();

  // Return the axis coordinates in pad units (pixels).
  double xminPad() const { return m_xminPad; }
  double xmaxPad() const { return m_xmaxPad; }
  double yminPad() const { return m_yminPad; }
  double ymaxPad() const { return m_ymaxPad; }

  // Return the axis coordinates in drawn units.
  double xmin() const { return m_xmin; }
  double xmax() const { return m_xmax; }
  double ymin() const { return m_ymin; }
  double ymax() const { return m_ymax; }

  // Return the pad.
  TVirtualPad* pad() const { return m_ppad; }

  // Return the first histogram for this pad.
  TH1* hist() const { return m_ph; }

  // Return the vertical mod lines associated with this pad.
  const std::vector<TLine*>& verticalModLines() const { return m_vmlLines; }

  // Update the coordinates and histogram for this pad.
  int update();

  // Set the histogram range
  int setRangeX(double x1, double x2);
  int setRangeY(double y1, double y2);
  int setRanges(double x1, double x2, double y1, double y2);

  // Add top and right axis using attributes of the first histogram on the pad.
  int addAxis();

  // Add top x-axis with attributes taken from the first histogram on the pad.
  int addAxisTop();

  // Add top x-axis with specified attributes.
  int addAxisTop(double ticksize, int ndiv);

  // Draw the top axis.
  int drawAxisTop();

  // Add right y-axis with attributes from the first histogram on the pad.
  int addAxisRight();

  // Draw right y-axis with specified attributes.
  int addAxisRight(double ticksize, int ndiv);

  // Draw right y-axis.
  int drawAxisRight();

  // Add vertical modulus lines.
  // I.e at x = xoff, xoff+/-xmod, xoff+/-2*xmod, ...
  int addVerticalModLines(double xmod, double xoff =0.0);

  // Draw the current mod lines.
  int drawVerticalModLines();

  // Fix the BG color for 2D histos to be the same as the lowest color.
  // Otherwise underflows have the color of zeros.
  int fixFrameFillColor();

private:

  TVirtualPad* m_ppad;
  double m_xminPad;
  double m_xmaxPad;
  double m_yminPad;
  double m_ymaxPad;
  double m_xmin;
  double m_xmax;
  double m_ymin;
  double m_ymax;
  TH1* m_ph;
  bool m_top;
  double m_topTicksize;
  double m_topNdiv;
  bool m_right;
  double m_rightTicksize;
  double m_rightNdiv;
  double m_vmlXmod;
  double m_vmlXoff;
  std::vector<TLine*> m_vmlLines;

};

#endif