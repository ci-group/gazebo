#ifndef MODEL_LIST_WIDGET_HH
#define MODEL_LIST_WIDGET_HH

#include <QWidget>

#include "sdf/sdf.h"
#include "msgs/msgs.h"
#include "transport/TransportTypes.hh"
#include "rendering/RenderTypes.hh"

class QTreeWidget;
class QTreeWidgetItem;
class QPushButton;
class QtTreePropertyBrowser;
class QtVariantPropertyManager;
class QtProperty;
class QtTreePropertyItem;
class QtBrowserItem;
class QtVariantEditorFactory;

namespace boost
{
  class recursive_mutex;
}

namespace gazebo
{
  namespace gui
  {
    class ModelEditWidget;

    class ModelListWidget : public QWidget
    {
      Q_OBJECT
      public: ModelListWidget( QWidget *parent = 0 );
      public: virtual ~ModelListWidget();

      private slots: void OnModelSelection(QTreeWidgetItem *item, int column);
      private slots: void Update();
      private slots: void OnPropertyChanged(QtProperty *_item);
      private slots: void OnMoveTo();
      private slots: void OnDelete();
      private slots: void OnFollow();
      private slots: void OnCustomContextMenu(const QPoint &_pt);
      private slots: void OnCurrentPropertyChanged(QtBrowserItem *);


      private: void OnEntities( const boost::shared_ptr<msgs::Entities const> &_msg );
      private: void OnEntity( const boost::shared_ptr<msgs::Entity const> &_msg );

      private: void OnEntityInfo( const boost::shared_ptr<msgs::Factory const> &_msg );

      private: void ProcessEntity( const msgs::Entity &_msg );

      private: void FillPropertyTree(sdf::ElementPtr &_elem,
                                     QtProperty *_parentItem);

      private: void FillSDF( QtProperty *_item, sdf::ElementPtr &_elem,
                             QtProperty *_changedItem );

      private: QtProperty *PopChildItem(QList<QtProperty*> &_list,
                                        const std::string &_name);
      private: QtProperty *GetChildItem(QtProperty *_item, 
                                        const std::string &_name);

      private: QTreeWidget *modelTreeWidget;
      private: QtTreePropertyBrowser *propTreeBrowser;
      private: QAction *moveToAction;
      private: QAction *deleteAction;
      private: QAction *followAction;

      private: transport::NodePtr node;
      private: transport::PublisherPtr entitiesRequestPub, entityPub;
      private: transport::PublisherPtr selectionPub, factoryPub;
      private: transport::SubscriberPtr entitiesSub, newEntitySub,entityInfoSub ;

      private: rendering::VisualPtr modelVisual;
      private: std::list<rendering::VisualPtr> visuals;
      private: sdf::SDFPtr modelSDF;

      private: ModelEditWidget *modelEditWidget;
      private: QtVariantPropertyManager *variantManager;
      private: QtVariantEditorFactory *variantFactory;
      private: boost::recursive_mutex *propMutex;
      private: sdf::ElementPtr sdfElement;
      private: std::string selectedModelName;
      private: bool fillingPropertyTree;
      private: QtProperty *selectedProperty;
    };
  }
}
#endif
