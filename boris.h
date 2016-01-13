/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/***************************************************************************
 *            boris.h
 *
 *  Tue Nov 26 16:56:00 CEST 2013
 *  Copyright 2013 Lars Muldjord
 *  muldjordlars@gmail.com
 ****************************************************************************/

/*
 *  This file is part of Boris.
 *
 *  Boris is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Boris is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Boris; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */
#ifndef _BORIS_H
#define _BORIS_H

#include "behaviour.h"
#include "stats.h"

#include <QGraphicsView>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QSoundEffect>
#include <QAction>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QMenu>

class Boris : public QGraphicsView
{
  Q_OBJECT;
public:
  Boris(QList<Behaviour> *behaviours, QWidget *parent);
  ~Boris();
  QTimer behavTimer;
  QList<Behaviour> *behaviours;
  void changeSize(int newSize);
  void soundEnable(bool enabled);
  void statsEnable(bool enabled);
  void setIndependence(int value);
  int borisSize;
  void collide(Boris *b);
  Boris *boris;
  int getHygiene();
              
public slots:
  void nextFrame();
  void earthquake();
  void walkUp();
  void walkDown();
  void walkLeft();
  void walkRight();
  
protected:
  void mouseMoveEvent(QMouseEvent* event);
  void mousePressEvent(QMouseEvent* event);
  void mouseReleaseEvent(QMouseEvent* event);
  void enterEvent(QEvent *event);
  void leaveEvent(QEvent *event);

private slots:
  void changeBehaviour(QString behav = "", int time = 0);
  void handleBehaviourChange(QAction* a);
  void handlePhysics();
  void statProgress();
  void statQueueProgress();

private:
  bool falling;
  bool grabbed;
  void moveBoris(int dX, int dY);
  void showBoris();
  void hideBoris();
  void createBehavMenu();
  int staticBehavs;
  int independence;
  
  int energy;
  int hunger;
  int bladder;
  int hygiene;
  int social;
  int fun;

  QMenu* bMenu;
  bool soundEnabled;
  int curFrame;
  int curBehav;
  QTimer animTimer;
  QTimer physicsTimer;
  QTimer statTimer;
  QTimer statQueueTimer;
  bool mMoving;
  QPoint mLastMousePosition;
  bool alreadyEvading;

  QGraphicsPixmapItem *sprite;
  QGraphicsPixmapItem *dirt;
  QPixmap origDirt;
  
  double vVel;
  double hVel;
  int alt;
  QPoint oldCursor;
  void sanityCheck();
  Stats *stats;
  bool showStats;
  QList<QString> behavQueue;
  int energyQueue;
  int hungerQueue;
  int bladderQueue;
  int socialQueue;
  int funQueue;
  int hygieneQueue;
  QString chooseFromCategory(QString category);
  void stopTimers();
  void startTimers();
  
};

#endif // _BORIS_H

// behaviours.at(1).
