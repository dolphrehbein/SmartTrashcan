
import UIKit

class TrashIndicator: UIImageView {
    
    public var trashIsFull = false {
        didSet { image = UIImage(named: trashIsFull ? "FullTrashIcon" : "TrashIcon" ) }
    }

}
